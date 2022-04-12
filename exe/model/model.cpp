﻿#include <components/physics_component.hpp>
#include <components/collider_component.hpp>
#include <components/movement_component.hpp>
#include <stb_image.h>
#include <default_app.hpp>
#include <file/obj.hpp>
#include <iostream>
#include <array>
#include <variant>
#include <experimental/generator>
#include <animation/animation.hpp>
#include <file/gltf__WIP.hpp>
#include <rnu/ecs/ecs.hpp>
#include <rnu/ecs/component.hpp>
#include <random>
#include <generic/primitives.hpp>

#include <vectors/font.hpp>

//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

static auto constexpr vs = R"(
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv[3];
layout(location = 5) in vec4 color;
layout(location = 6) in uvec4 joint_arr;
layout(location = 7) in vec4 weight_arr;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(std430, binding = 0) restrict readonly buffer Matrices
{
  mat4 view;
  mat4 proj;
};

layout(std430, binding = 3) restrict readonly buffer Transform
{
  mat4 transform;
  int animated;
};

layout(std430, binding = 1) restrict readonly buffer Joints
{
  mat4 joints[];
};

layout(location = 0) out vec3 pass_position;
layout(location = 1) out vec3 pass_normal;
layout(location = 2) out vec2 pass_uv[3];
layout(location = 5) out vec4 pass_color;
layout(location = 6) out vec3 pass_view_position;

void main()
{
  mat4 skinMat = (animated != 0) ? 
    (weight_arr.x * joints[int(joint_arr.x)] +
    weight_arr.y * joints[int(joint_arr.y)] +
    weight_arr.z * joints[int(joint_arr.z)] +
    weight_arr.w * joints[int(joint_arr.w)]) : mat4(1.0);

  vec3 pos = position;
  vec4 hom_position = transform * skinMat * vec4(pos, 1);
  pass_view_position = (view * hom_position).xyz;
  gl_Position = proj * view * hom_position;

  pass_position = hom_position.xyz / hom_position.w;
  pass_normal = vec3(inverse(transpose(mat4(mat3(transform * skinMat)))) * vec4(normal, 0));
  pass_uv[0] = uv[0];
  pass_uv[1] = uv[1];
  pass_uv[2] = uv[2];
  pass_color = color;
}
)";

static constexpr auto fs = R"(
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv[3];
layout(location = 5) in vec4 color;

layout(binding = 0) uniform sampler2D image_texture;
layout(location = 0) out vec4 out_color;

layout(std430, binding=2) restrict readonly buffer Mat
{
  vec4 mat_color;
  uint has_texture;
};

void main()
{
  vec2 ux = vec2(uv[0].x, uv[0].y);

  float light = max(0, dot(normalize(normal), normalize(vec3(1,1,1))));

  vec4 tex_color = (has_texture != 0) ? texture(image_texture, ux) : mat_color;
  
  if(tex_color.a < 0.6)
    discard;
  vec3 col = max(color.rgb, tex_color.rgb);
  col = col * vec3(0.5f, 0.7f, 1.0f) + col * light;
  out_color = vec4(col, 1);
}
)";

struct camera_component
{
  rnu::cameraf camera;
  rnu::projection<float> projection;
};

struct matrices
{
  rnu::mat4 view;
  rnu::mat4 proj;
};

struct material_component : rnu::component<material_component>
{
  goop::material material;
};

struct geometry_component : rnu::component<geometry_component>
{
  goop::geometry geometry;
  std::vector<goop::vertex_offset> parts;
};

struct parented_to_component : rnu::component<parented_to_component>
{
  rnu::weak_entity parent;
};

struct grouped_entity_component : rnu::component<grouped_entity_component>
{
  std::string name;
  std::vector<rnu::shared_entity> entities;
};

struct rigging_skeleton : rnu::component<rigging_skeleton>
{
  goop::transform_tree nodes;
  goop::skin skin;
  std::unordered_map<std::string, goop::joint_animation> animations;
  std::string active_animation;
  goop::mapped_buffer<rnu::mat4> joints;

  void run_animation(std::string anim)
  {
    auto const iter = animations.find(anim);
    if (iter != animations.end())
    {
      iter->second.start();
    }
    active_animation = std::move(anim);
  }
};

class rigging_system : public rnu::system {
public:
  rigging_system()
  {
    add_component_type<rigging_skeleton>();
    add_component_type<goop::transform_component>(rnu::component_flag::optional);
  }

  virtual void update(duration_type delta, rnu::component_base** components) const
  {
    auto* rig = static_cast<rigging_skeleton*>(components[0]);
    auto* transform = static_cast<goop::transform_component*>(components[1]);

    auto const iter = rig->animations.find(rig->active_animation);
    if (iter != rig->animations.end())
    {
      rig->nodes.animate(iter->second, delta);
      auto& mats = rig->skin.apply_global_transforms(rig->nodes);
      rig->joints->write(mats);
    }
  }
};

class object_render_system : public rnu::system
{
public:
  object_render_system()
    : _draw_state(nullptr)
  {
    add_component_type<goop::transform_component>();
    add_component_type<material_component>();
    add_component_type<geometry_component>();
    add_component_type<parented_to_component>(rnu::component_flag::optional);
  }

  void set_draw_state(goop::draw_state& state)
  {
    _draw_state = &state;
  }

  virtual void update(duration_type delta, rnu::component_base** components) const
  {
    auto* transform = static_cast<goop::transform_component*>(components[0]);
    auto* material = static_cast<material_component*>(components[1]);
    auto* geometry = static_cast<geometry_component*>(components[2]);
    auto* rig = static_cast<parented_to_component*>(components[3]);

    rigging_skeleton* skeleton = !rig ? nullptr :
      rig->parent.lock()->get<rigging_skeleton>();
    goop::transform_component* skeleton_tf = !rig ? nullptr :
      rig->parent.lock()->get<goop::transform_component>();

    if (skeleton)
      skeleton->joints->bind(*_draw_state, 1);

    rnu::mat4 model_mat = transform->create_matrix();
    if (skeleton_tf)
      model_mat = skeleton_tf->create_matrix() * model_mat;

    _object_transform->write(object_transform{
      .transform = model_mat,
      .animated = skeleton && skeleton->animations.contains(skeleton->active_animation)
      });
    material->material.bind(*_draw_state);
    _object_transform->bind(*_draw_state, 3);
    geometry->geometry->draw(*_draw_state, geometry->parts);
  }

private:
  struct object_transform
  {
    rnu::mat4 transform;
    int32_t animated;
  };
  mutable goop::mapped_buffer<object_transform> _object_transform;
  goop::draw_state* _draw_state;
};

rnu::shared_entity load_gltf_as_entity(std::filesystem::path const& path, rnu::ecs& ecs, goop::default_app& app, goop::geometry& main_geometry)
{
  auto gltf = goop::load_gltf(path, app.default_texture_provider(), main_geometry);
  auto const skeleton = ecs.create_entity_shared(
    grouped_entity_component{
      .name = "Rig"
    }
  );
  if (!gltf.skins.empty())
  {
    skeleton->add(rigging_skeleton{
      .nodes = gltf.nodes,
      .skin = gltf.skins[0],
      .animations = gltf.animations,
      .active_animation = ""
      });
  }
  auto* group = skeleton->get<grouped_entity_component>();

  for (size_t i = 0; i < gltf.materials.size(); ++i)
  {
    auto& geoms = gltf.material_geometries[i];
    group->entities.push_back(
      ecs.create_entity_shared(
        goop::transform_component{ },
        material_component{ .material = gltf.materials[i] },
        geometry_component{ .geometry = main_geometry, .parts = geoms },
        parented_to_component{ .parent = skeleton }
      )
    );
  }
  return skeleton;
}

rnu::vec3 projection(rnu::vec3 r, rnu::vec3 d)
{
  return d - dot(d, r) * r;
}

void swing_twist_decomposition(rnu::quat const& rotation,
  const rnu::vec3& direction,
  rnu::quat& swing,
  rnu::quat& twist)
{
  rnu::vec3 ra(rotation.x, rotation.y, rotation.z); // rotation axis
  rnu::vec3 p = projection(direction, ra); // return projection v1 on to v2  (parallel component)
  rnu::vec4 n(rotation.w, p.x, p.y, p.z);
  n = normalize(n);
  twist = { n.r, n.g, n.b, n.a };
  swing = rotation * conj(twist);
}

#include <vectors/vectors.hpp>
#include <vectors/distance.hpp>
#include <vectors/lines.hpp>

std::vector<goop::lines::line> subsampled_shape(goop::vector_image const& image, float base_subsampling)
{
  struct path_visitor
  {
    std::vector<goop::lines::line> segments;
    rnu::vec2d cursor{ 0, 0 };
    bool relative = false;
    float base_subsampling;
    rnu::vec2 last_curve_control;
    rnu::vec2 last_move_target;

    rnu::vec2d offset() const {
      return relative ? cursor : rnu::vec2d(0, 0);
    }

    void operator()(goop::line_data_t const& data) {
      segments.push_back({ .start = cursor, .end = data.value + offset() });
    }

    void operator()(goop::vertical_data_t const& data) {
      auto end = cursor;
      end.y = offset().y + data.value;
      segments.push_back({ .start = cursor, .end = end });
    }

    void operator()(goop::horizontal_data_t const& data) {
      auto end = cursor;
      end.x = offset().x + data.value;
      segments.push_back({ .start = cursor, .end = end });
    }

    void operator()(goop::smooth_quad_bezier_data_t const& data)
    {
      auto to_last = last_curve_control - cursor;
      auto next = cursor - to_last;

      auto const off = offset();
      goop::lines::bezier curve{
        .start = cursor,
        .control = next,
        .end = {off.x + data.value.x, off.y + data.value.y}
      };
      last_curve_control = curve.control;
      goop::lines::subsample(curve, base_subsampling, segments);
    }

    void operator()(goop::smooth_curve_to_data_t const& data)
    {
      auto to_last = last_curve_control - cursor;
      auto next = cursor - to_last;

      auto const off = offset();
      goop::lines::curve curve{
        .start = cursor,
        .control_start = next,
        .control_end = {off.x + data.x2, off.y + data.y2},
        .end = {off.x + data.x, off.y + data.y}
      };
      last_curve_control = curve.control_end;
      goop::lines::subsample(curve, base_subsampling, segments);
    }

    void operator()(goop::quad_bezier_data_t const& data)
    {
      auto const off = offset();
      goop::lines::bezier curve{
        .start = cursor,
        .control = {off.x + data.x1, off.y + data.y1},
        .end = {off.x + data.x, off.y + data.y}
      };
      last_curve_control = curve.control;
      goop::lines::subsample(curve, base_subsampling, segments);
    }

    void operator()(goop::curve_to_data_t const& data)
    {
      auto const off = offset();
      goop::lines::curve curve{
        .start = cursor,
        .control_start = {off.x + data.x1, off.y + data.y1},
        .control_end = {off.x + data.x2, off.y + data.y2},
        .end = {off.x + data.x, off.y + data.y}
      };
      last_curve_control = curve.control_end;
      goop::lines::subsample(curve, base_subsampling, segments);
    }

    void operator()(goop::arc_data_t const& data)
    {
      auto const off = offset();
      goop::lines::arc curve{
        .start = cursor,
        .radii = {data.rx, data.ry},
        .end = {off.x + data.x, off.y + data.y},
        .rotation = float(data.x_axis_rotation),
        .large_arc = data.large_arc_flag != 0,
        .sweep = data.sweep_flag != 0
      };
      goop::lines::subsample(curve, base_subsampling, segments);
    }

    void operator()(goop::close_data_t const& close)
    {
      segments.push_back({ .start = cursor, .end = last_move_target });
    }

    void operator()(goop::move_data_t const& move)
    {
      last_move_target = move.value + offset();
    }
  } visitor;
  visitor.base_subsampling = base_subsampling;

  for (auto const& seg : image.path())
  {
    visitor.relative = is_relative(seg.type);
    goop::visit(seg, visitor);
    goop::move_cursor(seg, visitor.cursor);
  }

  return visitor.segments;
}
float minimum_distance(rnu::vec2 v, rnu::vec2 w, rnu::vec2 p) {
  // Return minimum distance between line segment vw and point p
  auto const diff = w - v;
  const float l2 = dot(diff, diff);  // i.e. |w-v|^2 -  avoid a sqrt
  if (l2 == 0.0) return rnu::norm(p - v);   // v == w case
  // Consider the line extending the segment, parameterized as v + t (w - v).
  // We find projection of point p onto the line. 
  // It falls where t = [(p-v) . (w-v)] / |w-v|^2
  // We clamp t from [0,1] to handle points outside the segment vw.
  const float t = rnu::max(0, rnu::min(1, dot(p - v, w - v) / l2));
  const rnu::vec2 projection = v + t * (w - v);  // Projection falls on the segment
  return rnu::norm(p - projection);
}

int const scanline_test(rnu::vec2 a, rnu::vec2 b, rnu::vec2 r)
{
  auto const s = b - a;
  auto const n = rnu::vec2(-s.y, s.x);
  auto const k = a - r;
  auto const t = (-k.y) / (s.y);

  auto const cross = [](auto x, auto y) { return x.x * y.y - x.y * y.x; };
  auto const u = (cross(k, s)) / s.y;

  bool const intersects = u > 0 && t >= 0 && t <= 1;

  if (!intersects)
    return 0;

  return rnu::sign(dot(n, k));
}

float signed_distance(std::vector<goop::lines::line> const& polygon, rnu::vec2 point)
{
  float dmin = std::numeric_limits<float>::max();
  int intersections = 0;

  for (auto const& line : polygon)
  {
    auto const d = minimum_distance(line.start, line.end, point + 1e-4);
    intersections += scanline_test(line.start, line.end, point + 1e-4);

    if (abs(d) < abs(dmin))
      dmin = d;
  }
  return (intersections == 0 ? 1 : -1) * dmin;
}

void emplace(std::vector<goop::lines::line> const& polygon, goop::rect const& r, float scale, int padding, float sdfw, std::vector<std::uint8_t>& img, int iw, int ih, int x, int y)
{
  auto const at = [&](int x, int y) -> std::uint8_t& { return img[x + y * iw]; };

  float const w = r.max.x - r.min.x + padding * 2;
  float const h = r.max.y - r.min.y + padding * 2;

  for (int i = 0; i < std::ceil(w); ++i)
  {
    for (int j = 0; j < std::ceil(h); ++j)
    {
      auto const signed_distance = ::signed_distance(polygon, { (i + r.min.x - padding) / scale, (j + r.min.y - padding) / scale }) * scale;
      auto min = -sdfw;
      auto max = sdfw;

      auto const iix = x + i + r.min.x - padding;
      auto const iiy = y + j + r.min.y - padding;

      at(iix, iiy) = std::max<float>(at(iix, iiy), (1 - std::clamp((signed_distance - min) / (max - min), 0.0f, 1.0f)) * 255);
    }
  }
}

namespace goop
{
  struct set_glyph
  {
    glyph_id glyph;
    rnu::vec2 offset;
    rect bounds;
  };

  std::vector<set_glyph> text_set(goop::font_file const& font, std::wstring_view str, float const em_size, rnu::vec2 cursor)
  {
    thread_local static std::vector<goop::glyph_id> glyphs;
    glyphs.clear();
    for (auto& c : str)
    {
      auto const gly = font.glyph_index(c);
      glyphs.push_back(gly ? *gly : goop::glyph_id{ 0 });
    }

    bool has_substituted = true;
    auto const font_scale = em_size / font.units_per_em();

    while (has_substituted)
    {
      has_substituted = false;
      for (int i = 0; i < glyphs.size() - 1; ++i)
      {
        auto const second = glyphs.size() - 1 - i;
        auto const first = second - 1;

        auto sub = i < glyphs.size() - 2 ? font.try_substitution(std::array{ glyphs[first - 1], glyphs[first], glyphs[second] }) : std::nullopt;
        if (!sub)
          sub = font.try_substitution(std::array{ glyphs[first], glyphs[second] });
        if (sub)
        {
          has_substituted = true;
          glyphs[first] = *sub;
          glyphs.erase(glyphs.begin() + second);
        }
      }
    }

    std::vector<set_glyph> set_glyphs(glyphs.size());

    auto const basey = 40;
    auto const rad = 0.5f;

    for (int i = 0; i < glyphs.size(); ++i)
    {
      auto const gly = glyphs[i];
      auto& result = set_glyphs[i];
      result.glyph = gly;
      auto const rec = font.get_rect(gly);

      auto const next_glyph = i < glyphs.size() - 1 ? glyphs[i + 1] : goop::glyph_id{ 0 };
      auto const [ad1, be1] = font.advance_bearing(gly, next_glyph);

      result.bounds = rec;
      result.bounds.min *= font_scale;
      result.bounds.max *= font_scale;
      result.offset = cursor + rnu::vec2(be1 * font_scale, 0);

      cursor.x += ad1 * font_scale;
    }

    return set_glyphs;
  }
}

int main()
{
  goop::font_file fnt("../../../../../res/BIZUDPGothic-Regular.ttf");

  std::vector<goop::lines::line_segment> outline;
  auto const load_letter = [&](std::optional<goop::glyph_id> ch) {
    goop::rect bounds;
    outline.clear();
    auto const gly = *ch;
    fnt.outline(gly, outline, bounds);

    std::vector<goop::lines::line> letter;
    for (auto const& o : outline)
      std::visit([&](auto const& x) { goop::lines::subsample(x, 10, letter); }, o);
    return letter;
  };

  auto const load_svg = [](auto&& path) {
    goop::vector_image img;
    img.parse(path);
    return subsampled_shape(img, 10);
  };

  auto const i0 = load_svg("M12,21.35L10.55,20.03C5.4,15.36 2,12.27 2,8.5C2,5.41 4.42,3 7.5,3C9.24,3 10.91,3.81 12,5.08C13.09,3.81 14.76,3 16.5,3C19.58,3 22,5.41 22,8.5C22,12.27 18.6,15.36 13.45,20.03L12,21.35Z");
  auto const i1 = load_svg("M12.1,18.55L12,18.65L11.89,18.55C7.14,14.24 4,11.39 4,8.5C4,6.5 5.5,5 7.5,5C9.04,5 10.54,6 11.07,7.36H12.93C13.46,6 14.96,5 16.5,5C18.5,5 20,6.5 20,8.5C20,11.39 16.86,14.24 12.1,18.55M16.5,3C14.76,3 13.09,3.81 12,5.08C10.91,3.81 9.24,3 7.5,3C4.42,3 2,5.41 2,8.5C2,12.27 5.4,15.36 10.55,20.03L12,21.35L13.45,20.03C18.6,15.36 22,12.27 22,8.5C22,5.41 19.58,3 16.5,3Z");
  auto const i2 = load_svg("M8,5.14V19.14L19,12.14L8,5.14Z");
  auto const i3 = load_svg("M9,5A4,4 0 0,1 13,9A4,4 0 0,1 9,13A4,4 0 0,1 5,9A4,4 0 0,1 9,5M9,15C11.67,15 17,16.34 17,19V21H1V19C1,16.34 6.33,15 9,15M16.76,5.36C18.78,7.56 18.78,10.61 16.76,12.63L15.08,10.94C15.92,9.76 15.92,8.23 15.08,7.05L16.76,5.36M20.07,2C24,6.05 23.97,12.11 20.07,16L18.44,14.37C21.21,11.19 21.21,6.65 18.44,3.63L20.07,2Z");
  auto const i4 = load_svg("M22.11 21.46L2.39 1.73L1.11 3L5.2 7.09C3.25 7.5 1.85 9.27 2 11.31C2.12 12.62 2.86 13.79 4 14.45V16C4 16.55 4.45 17 5 17H7V14.88C5.72 13.58 5 11.83 5 10C5 9.11 5.18 8.23 5.5 7.4L7.12 9C6.74 10.84 7.4 12.8 9 14V16C9 16.55 9.45 17 10 17H14C14.31 17 14.57 16.86 14.75 16.64L17 18.89V19C17 19.34 16.94 19.68 16.83 20H18C18.03 20 18.06 20 18.09 20L20.84 22.73L22.11 21.46M9.23 11.12L10.87 12.76C10.11 12.46 9.53 11.86 9.23 11.12M13 15H11V12.89L13 14.89V15M10.57 7.37L9.13 5.93C10.86 4.72 13.22 4.67 15 6C16.26 6.94 17 8.43 17 10C17 11.05 16.67 12.05 16.08 12.88L14.63 11.43C14.86 11 15 10.5 15 10C15 8.34 13.67 7 12 7C11.5 7 11 7.14 10.57 7.37M17.5 14.31C18.47 13.09 19 11.57 19 10C19 8.96 18.77 7.94 18.32 7C19.63 7.11 20.8 7.85 21.46 9C22.57 10.9 21.91 13.34 20 14.45V16C20 16.22 19.91 16.42 19.79 16.59L17.5 14.31M10 18H14V19C14 19.55 13.55 20 13 20H11C10.45 20 10 19.55 10 19V18M7 19C7 19.34 7.06 19.68 7.17 20H6C5.45 20 5 19.55 5 19V18H7V19Z");

  {
    int iw = 1024;
    int ih = 1024;
    std::vector<std::uint8_t> img(iw * ih);

    emplace(i0, goop::rect{ {0,0}, {24, 24}}, 1, 5, 1.f, img, iw, ih, 8, 8);
    emplace(i1, goop::rect{ {0,0}, {24, 24}}, 1, 5, 1.f, img, iw, ih, 8, 8 + 24);
    emplace(i2, goop::rect{ {0,0}, {24, 24}}, 1, 5, 1.f, img, iw, ih, 8, 8 + 24 + 24);
    emplace(i3, goop::rect{ {0,0}, {24, 24}}, 1, 5, 1.f, img, iw, ih, 8, 8 + 24 + 24 + 24);
    emplace(i4, goop::rect{ {0,0}, {24, 24}}, 1, 5, 1.f, img, iw, ih, 8, 8 + 24 + 24 + 24 + 24);

    auto const size = 16;
    auto const scale = size / fnt.units_per_em();

    auto const rad = 0.5f;

    for (auto const& set : text_set(fnt, L"Welch fieser Fön mit Spaß 僕の部屋は元気でした я люблю яблока", size, { 40, 40 }))
    {
      auto const outline = load_letter(set.glyph);

      emplace(outline, set.bounds, scale, 2, rad, img, iw, ih, set.offset.x, set.offset.y);
    }

    stbi_write_png("../../../../../result.png", iw, ih, 1, img.data(), 0);
  }

  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
  goop::default_app app("Model");
  glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    std::cout << message << '\n';
    }, nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);

  rnu::ecs ecs;
  std::vector<rnu::shared_entity> entities;

  rigging_system rigging_system;
  object_render_system object_renderer;
  goop::physics_system physics_system;
  goop::collision_collector_system collision_collector;

  rnu::system_list graphics_list;
  graphics_list.add(rigging_system);
  graphics_list.add(object_renderer);

  rnu::system_list physics_list;
  graphics_list.add(collision_collector);
  graphics_list.add(physics_system);

  goop::geometry main_geometry;
  goop::geometry collider_geometry;
  collider_geometry->set_display_type(goop::display_type::outlines);

  rnu::shared_entity ptcl = load_gltf_as_entity("../../../../../res/ptcl/scene.gltf", ecs, app, main_geometry);
  ptcl->add(goop::transform_component{ .position = rnu::vec3(0, 15, 0.0), .scale = rnu::vec3(2.f) });
  ptcl->add(goop::collider_component{ .shape = goop::sphere_collider{.center_offset = rnu::vec3{0, 1.0f, 0}, .radius = 1.f}, .movable = true });
  ptcl->add(goop::physics_component{ .acceleration = {0, -9.81, 0} });
  ptcl->get<rigging_skeleton>()->run_animation("Idle");

  std::vector<rnu::shared_entity> balls;

  std::mt19937 gen;
  std::uniform_real_distribution<float> dist(-100, 100);
  std::uniform_real_distribution<float> dists(0.3, 16.0);

  goop::vertex plane[]
  {
    goop::vertex{.position = {-100, 0, -100}, .normal = {0, 1, 0}, .uv = {{0, 0}}, .color = {0,0,0, 255} },
    goop::vertex{.position = {-100, 0, 100}, .normal = {0, 1, 0}, .uv = {{0, 100}}, .color = { 0,0,0, 255} },
    goop::vertex{.position = {100, 0, -100}, .normal = {0, 1, 0}, .uv = {{100, 0}}, .color = {0,0,0, 255} },
    goop::vertex{.position = {100, 0, 100}, .normal = {0, 1, 0}, .uv = {{100, 100}}, .color = {0,0,0, 255} },
  };
  std::uint32_t indices[]{
    0, 1, 2, 2, 1, 3
  };
  auto const tex = [&](auto path) {
    int img_w, img_h, img_comp;
    stbi_uc* data = stbi_load(path, &img_w, &img_h, &img_comp, 4);

    goop::texture texture = app.default_texture_provider().acquire(
      goop::texture_type::t2d, goop::data_type::rgba8unorm, img_w, img_h, goop::compute_texture_mipmap_count);
    texture->set_data(0, 0, 0, img_w, img_h, 4, { data, data + img_w * img_h * 4 });
    texture->generate_mipmaps();

    stbi_image_free(data);
    return texture;
  };
  auto const geom = main_geometry->append_vertices(plane, indices);

  goop::material plane_mat{
    .diffuse_map = tex("../../../../../res/grass.jpg"),
    .data = {
      .color = {1,1,1, 1.0},
      .has_texture = true,
    }
  };

  goop::material xmat{
    .data = {
      .color = {1,0, 0, 1.0},
      .has_texture = 0,
    }
  };

  auto sphere = goop::make_sphere(2);
  auto comp = ecs.create_entity_unique();
  comp->add(goop::transform_component{ .position = rnu::vec3(0, 8, 0) });
  comp->add(geometry_component{ .geometry = collider_geometry, .parts = { collider_geometry->append_vertices(sphere.vertices, goop::to_line_sequence(sphere.indices)) } });
  comp->add(material_component{ .material = xmat });

  for (int i = 0; i < 100; ++i)
  {
    rnu::shared_entity& ball = balls.emplace_back(load_gltf_as_entity("../../../../../res/ballymcballface/scene.gltf", ecs, app, main_geometry));
    ball->add(goop::transform_component{ .position = rnu::vec3(dist(gen), 0.5f, dist(gen)), .scale = rnu::vec3(dists(gen)) });
    ball->add(goop::collider_component{ .shape = goop::sphere_collider{.center_offset = rnu::vec3{0, 0, 0}, .radius = 1.f} });
  }

  auto coll = ecs.create_entity_shared(goop::transform_component{ .rotation = rnu::quat(rnu::vec3(0, 0, 1), 0.0f) },
    goop::collider_component{ .shape = goop::plane_collider{} }, geometry_component{ .geometry = main_geometry, .parts = {geom} }, material_component{ .material = plane_mat });

  goop::sampler sampler;
  sampler->set_clamp(goop::wrap_mode::repeat);
  sampler->set_min_filter(goop::sampler_filter::linear, goop::sampler_filter::linear);
  sampler->set_mag_filter(goop::sampler_filter::linear);
  sampler->set_max_anisotropy(16);

  std::string shader_log;
  goop::shader_pipeline pipeline;
  goop::shader vert(goop::shader_type::vertex, vs, &shader_log);
  std::cout << "VSLOG: " << shader_log << '\n';
  goop::shader frag(goop::shader_type::fragment, fs, &shader_log);
  std::cout << "FSLOG: " << shader_log << '\n';
  pipeline->use(vert);
  pipeline->use(frag);
  goop::mapped_buffer<matrices> matrix_buffer{ 1 };

  int img = 0;

  goop::input_key anim_key(app.window().get(), GLFW_KEY_O);
  goop::input_key restart_anim(app.window().get(), GLFW_KEY_SPACE);
  goop::input_key key_u(app.window().get(), GLFW_KEY_UP);
  goop::input_key key_d(app.window().get(), GLFW_KEY_DOWN);
  goop::input_key key_l(app.window().get(), GLFW_KEY_LEFT);
  goop::input_key key_r(app.window().get(), GLFW_KEY_RIGHT);

  while (app.begin_frame())
  {
    auto [window_width, window_height] = app.default_draw_state()->current_surface_size();

    auto& state = app.default_draw_state();
    state->set_depth_test(true);
    pipeline->bind(state);

    matrix_buffer->write(matrices{
        app.default_camera().matrix(),
        rnu::cameraf::projection(rnu::radians(60.f), window_width / float(window_height), 0.01f, 1000.f)
      });
    matrix_buffer->bind(state, 0);

    switch (restart_anim.update())
    {
    case goop::input_key::state::press:
      //ptcl->get<rigging_skeleton>()->run_animation("Running");
      break;
    case goop::input_key::state::release:
      //ptcl->get<rigging_skeleton>()->run_animation("Idle");
      break;
    case goop::input_key::state::down:
      if (ptcl->get<goop::physics_component>()->on_floor)
        ptcl->get<goop::physics_component>()->velocity.y = 20.0;
      break;
    }

    auto const q = app.default_camera().rotation();
    float theta = std::atan2f(q.y, q.w);
    ptcl->get<goop::transform_component>()->rotation = rnu::quat(cosf(theta), 0, sinf(theta), 0);

    auto fwd = ptcl->get<goop::transform_component>()->rotation * rnu::vec3(0, 0, -1);
    auto right = ptcl->get<goop::transform_component>()->rotation * rnu::vec3(1, 0, 0);
    auto rfac = 10 * (glfwGetKey(app.window().get(), GLFW_KEY_RIGHT) - glfwGetKey(app.window().get(), GLFW_KEY_LEFT));
    auto ffac = -10 * (glfwGetKey(app.window().get(), GLFW_KEY_DOWN) - glfwGetKey(app.window().get(), GLFW_KEY_UP));

    auto vec = fwd * ffac + right * rfac;
    vec.y = ptcl->get<goop::physics_component>()->velocity.y;

    ptcl->get<goop::physics_component>()->velocity = vec;
    ptcl->get<goop::physics_component>()->acceleration = { 0, -98.1, 0 };

    ecs.update(app.current_delta_time(), physics_list);

    sampler->bind(state, 0);
    object_renderer.set_draw_state(app.default_draw_state());
    ecs.update(app.current_delta_time(), graphics_list);
    img = app.end_frame();
  }
}