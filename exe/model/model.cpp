#include "ui/element.hpp"
#include "ui/frame.hpp"
#include "ui/label.hpp"

#include "gui_test.hpp"
#include "vector_graphics.hpp"
#include "sdf_font.hpp"
#include "text.hpp"
#include "panel_2d.hpp"
#include "atlas_cache.hpp"

#include <components/physics_component.hpp>
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
#include <file/gltf.hpp>
#include <rnu/ecs/ecs.hpp>
#include <rnu/ecs/component.hpp>
#include <random>
#include <generic/primitives.hpp>
#include <rnu/thread_pool.hpp>
#include <map>
#include <vectors/font.hpp>
#include <rnu/utility.hpp>
#include <vectors/skyline_packer.hpp>
#include <vectors/character_ranges.hpp>
#include <geometry.hpp>

//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <model_shaders/model.fs.h>
#include <model_shaders/model.vs.h>

static std::filesystem::path res = std::filesystem::exists("res") ? "res" : RESOURCE_DIRECTORY;


//
//static auto constexpr vs = R"(
//#version 460 core
//
//layout(location = 0) in vec3 position;
//layout(location = 1) in vec3 normal;
//layout(location = 2) in vec2 uv[3];
//layout(location = 5) in vec4 color;
//layout(location = 6) in uvec4 joint_arr;
//layout(location = 7) in vec4 weight_arr;
//
//out gl_PerVertex
//{
//	vec4 gl_Position;
//};
//
//layout(std430, binding = 0) restrict readonly buffer Matrices
//{
//  mat4 view;
//  mat4 proj;
//};
//
//layout(std430, binding = 3) restrict readonly buffer Transform
//{
//  mat4 transform;
//  int animated;
//};
//
//layout(std430, binding = 1) restrict readonly buffer Joints
//{
//  mat4 joints[];
//};
//
//layout(location = 0) out vec3 pass_position;
//layout(location = 1) out vec3 pass_normal;
//layout(location = 2) out vec2 pass_uv[3];
//layout(location = 5) out vec4 pass_color;
//layout(location = 6) out vec3 pass_view_position;
//
//void main()
//{
//  mat4 skinMat = (animated != 0) ? 
//    (weight_arr.x * joints[int(joint_arr.x)] +
//    weight_arr.y * joints[int(joint_arr.y)] +
//    weight_arr.z * joints[int(joint_arr.z)] +
//    weight_arr.w * joints[int(joint_arr.w)]) : mat4(1.0);
//
//  vec3 pos = position;
//  vec4 hom_position = transform * skinMat * vec4(pos, 1);
//  pass_view_position = (view * hom_position).xyz;
//  gl_Position = proj * view * hom_position;
//
//  pass_position = hom_position.xyz / hom_position.w;
//  pass_normal = vec3(inverse(transpose(mat4(mat3(transform * skinMat)))) * vec4(normal, 0));
//  pass_uv[0] = uv[0];
//  pass_uv[1] = uv[1];
//  pass_uv[2] = uv[2];
//  pass_color = color;
//}
//)";
//
//static constexpr auto fs = R"(
//#version 460 core
//
//layout(location = 0) in vec3 position;
//layout(location = 1) in vec3 normal;
//layout(location = 2) in vec2 uv[3];
//layout(location = 5) in vec4 color;
//
//layout(binding = 0) uniform sampler2D image_texture;
//layout(location = 0) out vec4 out_color;
//
//layout(std430, binding=2) restrict readonly buffer Mat
//{
//  vec4 mat_color;
//  uint has_texture;
//};
//
//void main()
//{
//  vec2 ux = vec2(uv[0].x, uv[0].y);
//
//  float light = max(0, dot(normalize(normal), normalize(vec3(1,1,1))));
//
//  vec4 tex_color = (has_texture != 0) ? texture(image_texture, ux) : mat_color;
//  
//  if(tex_color.a < 0.6)
//    discard;
//  vec3 col = max(color.rgb, tex_color.rgb);
//  col = col * vec3(0.5f, 0.7f, 1.0f) + col * light;
//  out_color = vec4(col, 1);
//}
//)";

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
  thread_local std::unordered_map<std::filesystem::path, std::optional<goop::gltf_data>> datas;

  std::optional<goop::gltf_data> data;
  if (datas.contains(path))
    data = datas[path];
  else
  {
    data = datas[path] = goop::load_gltf(path, app.default_texture_provider(), main_geometry);
  }
  auto const& gltf = *data;

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


#include "panel_2d.hpp"
#include <vectors/vectors.hpp>
#include <vectors/distance.hpp>
#include <vectors/lines.hpp>
#include "symbol.hpp"

template<goop::lines::line_segment_sequence T, typename Data>
void emplace(T&& polygon, rnu::rect2f const& r, float scale, int padding, float sdfw, std::vector<Data>& img, int iw, int ih, int x, int y, rnu::vec2 voff = { 0,0 })
{
  auto const at = [&](int x, int y) -> Data& { return img[x + y * iw]; };

  //float const w = r.max.x - r.min.x + padding * 2;
  //float const h = r.max.y - r.min.y + padding * 2;
  auto const min_x = std::floor(r.position.x) - padding;
  auto const min_y = std::floor(r.position.y) - padding;
  auto const max_x = std::floor(r.position.x + r.size.x) + padding;
  auto const max_y = std::floor(r.position.y + r.size.y) + padding;

  for (int i = min_x; i <= max_x; ++i)
  {
    for (int j = min_y; j <= max_y; ++j)
    {
      auto const signed_distance = goop::lines::signed_distance(polygon, { (i + voff.x) / scale, (j + voff.y) / scale }) * scale;
      auto min = -sdfw;
      auto max = sdfw;

      auto const iix = x + i;
      auto const iiy = y + j;

      at(iix, iiy) = Data(
        (1 - std::clamp((signed_distance - min) / (max - min), 0.0f, 1.0f)) * 255
      );
    }
  }
}

template<typename T>
void copy_linear(std::span<T> dst, int dw, int dh, rnu::rect2f dst_rect, std::span<T const> src, int sw, int sh, rnu::rect2f src_rect)
{
  if (src_rect.size.x == 0 || src_rect.size.y == 0)
    return;
  if (dst_rect.size.x == 0 || dst_rect.size.y == 0)
    return;

  int dst_x_minf = dst_rect.position.x * dw;
  int dst_x_maxf = (dst_rect.position.x + dst_rect.size.x) * dw;
  int dst_y_minf = dst_rect.position.y * dh;
  int dst_y_maxf = (dst_rect.position.y + dst_rect.size.y) * dh;

  int dst_x_min = std::floor(dst_x_minf);
  int dst_x_max = std::ceilf(dst_x_maxf);
  int dst_y_min = std::floor(dst_y_minf);
  int dst_y_max = std::ceilf(dst_y_maxf);

  float src_x_min = src_rect.position.x * sw;
  float src_x_max = (src_rect.position.x + src_rect.size.x) * sw;
  float src_y_min = src_rect.position.y * sh;
  float src_y_max = (src_rect.position.y + src_rect.size.y) * sh;

  auto const get = [&](int x, int y) -> T const& {
    return src[x + y * sw];
  };
  auto const set = [&](int x, int y) -> T& {
    return dst[x + y * dw];
  };

  for (int x = dst_x_min; x <= dst_x_max; ++x)
  {
    for (int y = dst_y_min; y <= dst_y_max; ++y)
    {
      float const fac_x = src_x_min + (src_x_max - src_x_min) * ((x - dst_x_minf) / float(dst_x_maxf - dst_x_minf));
      auto const fract_x = std::fmodf(fac_x, 1.0f);
      float const fac_y = src_y_min + (src_y_max - src_y_min) * ((y - dst_y_minf) / float(dst_y_maxf - dst_y_minf));
      auto const fract_y = std::fmodf(fac_y, 1.0f);

      auto const src_x_0 = int(fac_x);
      auto const src_x_1 = src_x_0 + 1;
      auto const src_y_0 = int(fac_y);
      auto const src_y_1 = src_y_0 + 1;

      auto const s00 = get(src_x_0, src_y_0);
      auto const s10 = get(src_x_1, src_y_0);
      auto const s01 = get(src_x_0, src_y_1);
      auto const s11 = get(src_x_1, src_y_1);

      auto const s0 = rnu::cx::mix<float>(s00, s10, fract_x);
      auto const s1 = rnu::cx::mix<float>(s01, s11, fract_x);

      //auto const width = (18 / 18.f) * 16.f / 255;

      auto const final_value = rnu::cx::mix<float>(s0, s1, fract_y) / 255.0f;

      //(signed_distance - min) / (max - min)

      auto const d = -(final_value * (2 * 8.f) - 8.f) / (25.f / 14.f);

      auto const factor = rnu::smoothstep(0.9, -0.1, d) * 255;

      set(x, y) = std::max<uint8_t>(set(x, y), factor);
    }
  }
}




namespace goop::gui::el
{
  //class element_base;
  //using element = handle<element_base>;

  //enum class stretch_mode
  //{
  //  exact,
  //  at_most,
  //  undefined
  //};
  //enum class layout_mode
  //{
  //  match_parent,
  //  wrap_content
  //};
  //enum class item_justify
  //{
  //  start,
  //  middle,
  //  end
  //};
  //enum class orientation
  //{
  //  horizontal,
  //  vertical
  //};

  //struct layout_params
  //{
  //  std::variant<int, layout_mode> width = layout_mode::wrap_content;
  //  std::variant<int, layout_mode> height = layout_mode::wrap_content;
  //  std::any additional_data;
  //};

  //std::variant<int, layout_mode> get_mode(std::string_view str)
  //{
  //  if (str == "match_parent")
  //    return layout_mode::match_parent;
  //  else if (str == "wrap_content")
  //    return layout_mode::wrap_content;
  //  else
  //  {
  //    int size = 0;
  //    auto const result = std::from_chars(str.data(), str.data() + str.size(), size);
  //    if (result.ec == std::errc::invalid_argument)
  //      return 0;
  //    return size;
  //  }
  //}

  //std::string_view trimmed(std::string_view str)
  //{
  //  auto const begin = str.find_first_not_of(" \n\t\r");
  //  auto const end = str.find_last_not_of(" \n\t\r");
  //  
  //  if (begin == std::string_view::npos)
  //    return str;
  //  if (end == std::string_view::npos)
  //    return str.substr(begin);
  //  return str.substr(begin, end + 1);
  //}

  //layout_params to_layout(std::string_view str, std::any additional_data = {})
  //{
  //  auto const split_at = str.find_first_of(',');
  //  if (split_at == std::string_view::npos || split_at + 1 == str.size())
  //  {
  //    auto const mode = get_mode(str);
  //    return { mode, mode, additional_data };
  //  }

  //  auto const substr_first = trimmed(str.substr(0, split_at));
  //  auto const substr_second = trimmed(str.substr(split_at + 1));
  //  return { get_mode(substr_first), get_mode(substr_second), additional_data };
  //}

  //using layout_hint = std::pair<int, stretch_mode>;

  //static constexpr layout_hint layout_exact(int value) { return { value, stretch_mode::exact }; };
  //static constexpr layout_hint layout_at_most(int value) { return { value, stretch_mode::at_most }; };
  //static constexpr layout_hint layout_undefined(int value) { return { value, stretch_mode::undefined }; };

  //static texture_provider& provider() // Todo make remove
  //{
  //  static texture_provider p;
  //  return p;
  //}

  //class element_base
  //{
  //private:
  //  layout_hint _last_x_hint;
  //  layout_hint _last_y_hint;

  //public:
  //  void set_layout_params(layout_params params)
  //  {
  //    _layout_params = params;
  //    invalidate_layout();
  //  }

  //  layout_params const& get_layout_params()
  //  {
  //    return _layout_params;
  //  }

  //  void invalidate_draw()
  //  {
  //    _needs_redraw = true;
  //  }

  //  void invalidate_layout()
  //  {
  //    invalidate_draw();
  //    _needs_relayout = true;
  //  }

  //  virtual bool needs_relayout() const
  //  {
  //    return _needs_relayout;
  //  }

  //  virtual bool needs_redraw() const
  //  {
  //    return _needs_redraw;
  //  }

  //  void measure(layout_hint x_hint, layout_hint y_hint)
  //  {
  //    _last_x_hint = x_hint;
  //    _last_y_hint = y_hint;
  //    _measured_size = on_measure(x_hint, y_hint);
  //    set_measured_size(_measured_size);
  //  }

  //  rnu::vec4i margins() const
  //  {
  //    return _margin;
  //  }

  //  void set_margins(rnu::vec4 m)
  //  {
  //    _margin = m;
  //    invalidate_layout();
  //  }

  //  void layout(rnu::box<2, int> bounds)
  //  {
  //    auto const changed = _bounds != bounds;
  //    if (changed.any())
  //    {
  //      _bounds = bounds;
  //      _bounds.position += rnu::vec2i(_margin.x, _margin.y);

  //      _draw_buffer.set_position(_bounds.position);
  //      _draw_buffer.set_size(_bounds.size);
  //      _bounds.position = rnu::vec2i(0,0);

  //      if (_draw_bitmap)
  //        provider().free(*_draw_bitmap);

  //      _draw_bitmap = provider().acquire(texture_type::t2d,
  //        data_type::rgba8unorm, _bounds.size.x, _bounds.size.y, 1);
  //      _draw_render_target->bind_texture(0, *_draw_bitmap, 0);
  //      
  //      _draw_buffer.set_color({ 1,1,1,1 });
  //      _draw_buffer.set_texture(_draw_bitmap);
  //      _draw_buffer.set_uv({ 0,0 }, { 1,1 });
  //    }
  //    on_layout(changed.any(), _bounds);

  //    _needs_relayout = false;
  //  }

  //  rnu::vec2i measured_size() const 
  //  {
  //    return _measured_size;
  //  }

  //  rnu::vec2i measured_size_with_margins() const
  //  {
  //    return _measured_size + rnu::vec2i(_margin.x, _margin.y) + rnu::vec2i(_margin.z, _margin.w);
  //  }

  //  void render(draw_state_base& state, render_target& target)
  //  {
  //    // todo: if rebuild draw cache
  //    if (needs_redraw())
  //    {
  //      _draw_render_target->activate(state);
  //      _draw_render_target->clear_color(0, std::array{ 0.f,0.f,0.f,0.f });
  //      on_render(state, _draw_render_target);
  //    }

  //    target->activate(state);
  //    _draw_buffer.draw(state);
  //    target->deactivate(state);
  //  }

  //  virtual rnu::vec2i desired_size() const {
  //    return { 0, 0 };
  //  }

  //protected:
  //  virtual void on_layout(bool changed, rnu::box<2, int> bounds) {}
  //  virtual rnu::vec2i on_measure(layout_hint x_hint, layout_hint y_hint)
  //  {
  //    auto const size = desired_size();
  //    auto const& [w, w_mode] = x_hint;
  //    auto const& [h, h_mode] = y_hint;

  //    int width = 0;
  //    int height = 0;

  //    if (w_mode == stretch_mode::exact)
  //      width = w;
  //    else if (w_mode == stretch_mode::at_most)
  //      width = std::min(size.x, w);
  //    else
  //      width = size.x;

  //    if (h_mode == stretch_mode::exact)
  //      height = h;
  //    else if (h_mode == stretch_mode::at_most)
  //      height = std::min(size.y, h);
  //    else
  //      height = size.y;

  //    return { width, height };
  //  }
  //  virtual void on_render(draw_state_base& state, render_target& target) {};

  //private:
  //  void set_measured_size(rnu::vec2i size)
  //  {
  //    _measured_size = size;
  //  }

  //  bool _needs_relayout = true;
  //  bool _needs_redraw = true;
  //  layout_params _layout_params;
  //  rnu::vec2i _measured_size{0, 0};
  //  rnu::box<2, int> _bounds{};
  //  rnu::vec4i _margin{};

  //  render_target _draw_render_target;
  //  std::optional<texture> _draw_bitmap;
  //  panel_2d _draw_buffer;
  //};

  using namespace ui;

  void draw_root(element_base& el, draw_state_base& draw_state, render_target& target, int screen_width, int screen_height)
  {
    auto const size = el.measured_size();
    if (size.x != screen_width || size.y != screen_height)
      el.invalidate_layout();

    if (el.needs_relayout())
    {
      auto const& params = el.get_layout_params();
      el.measure(layout_exact(screen_width), layout_exact(screen_height));
      el.layout({ {0,0}, el.measured_size() });
    }

    el.render(draw_state, target);
  }

  //class frame_base : public element_base
  //{
  //public:
  //  using element_base::element_base;

  //  enum class background_scaling
  //  {
  //    stretch_fit,
  //    stretch_fill,
  //  };

  //  void set_background_scaling(background_scaling scaling)
  //  {
  //    _background_scaling = scaling;
  //    invalidate_layout();
  //  }

  //  void set_background(rnu::vec4 background)
  //  {
  //    _background.set_texture(std::nullopt);
  //    _background.set_color(background);
  //    _texture_size = { 0,0 };
  //  }
  //  void set_background(texture background)
  //  {
  //    if (!_has_texture)
  //    {
  //      invalidate_layout();
  //      _has_texture = true;
  //    }
  //    _background.set_color(rnu::vec4(1,1,1,1));
  //    _background.set_texture(background);
  //    _texture_size = background->dimensions();
  //  }

  //protected:
  //  rnu::vec2i desired_size() const override;
  //  void on_layout(bool changed, rnu::box<2, int> bounds) override;
  //  void on_render(draw_state_base& state, render_target& target) override;

  //private:
  //  bool _has_texture = false;
  //  rnu::vec2i _texture_size;
  //  panel_2d _background;
  //  background_scaling _background_scaling = background_scaling::stretch_fit;
  //};

  //rnu::vec2i frame_base::desired_size() const
  //{
  //  return _texture_size;
  //}

  //void frame_base::on_layout(bool changed, rnu::box<2, int> bounds)
  //{
  //  if (changed)
  //  {
  //    _background.set_position(bounds.position);
  //    _background.set_size(bounds.size);

  //    if (_has_texture)
  //    {
  //      auto const dst = bounds.size;
  //      auto const src = _texture_size;

  //      auto const dst_rel = dst.x / float(dst.y);
  //      auto const src_rel = src.x / float(src.y);
  //      switch (_background_scaling)
  //      {
  //      case background_scaling::stretch_fit:
  //      {
  //        // Fit inside, set uvs such that image fits fully inside of the bounds
  //        if (dst_rel > src_rel)
  //        {
  //          // limited by image height
  //          auto const height = 1.0;
  //          auto const width = dst_rel / src_rel;

  //          _background.set_uv({ 0.5f - width / 2, height / 2 + 0.5f }, { width / 2 + 0.5f, 0.5f - height / 2 });
  //        }
  //        else
  //        {
  //          // limited by image width
  //          auto const height = src_rel / dst_rel;
  //          auto const width = 1.0;

  //          _background.set_uv({ 0.5f - width / 2, height / 2 + 0.5f }, { width / 2 + 0.5f, 0.5f - height / 2 });
  //        }
  //      }
  //      break;
  //      case background_scaling::stretch_fill:
  //      {
  //        // Fit such that the image fills up the whole panel, centering it and cutting off overflows
  //        if (dst_rel > src_rel)
  //        {
  //          // limited by image height
  //          auto const height = src_rel / dst_rel;
  //          auto const width = 1.0;

  //          _background.set_uv({ 0.5f - width / 2, height / 2 + 0.5f }, { width / 2 + 0.5f, 0.5f - height / 2 });
  //        }
  //        else
  //        {
  //          // limited by image width
  //          auto const height = 1.0;
  //          auto const width = dst_rel / src_rel;

  //          _background.set_uv({ 0.5f - width / 2, height / 2 + 0.5f }, { width / 2 + 0.5f, 0.5f - height / 2 });
  //        }

  //      }
  //      break;
  //      }

  //      // Todo: background scaling...
  //    }
  //  }
  //}

  //void frame_base::on_render(draw_state_base& state, render_target& target)
  //{
  //  _background.draw(state);
  //}

  //class text_base : public frame_base
  //{
  //public:
  //  using frame_base::frame_base;

  //  void set_font(sdf_font font)
  //  {
  //    _text.set_font(font);
  //    invalidate_layout();
  //  }

  //  void set_color(rnu::vec4 color)
  //  {
  //    _text.set_color(color);
  //    invalidate_draw();
  //  }

  //  void set_text(std::wstring_view text)
  //  {
  //    _text.set_text(text);
  //    invalidate_layout();
  //  }

  //  void set_size(float size)
  //  {
  //    _text.set_size(size);
  //    invalidate_layout();
  //  }

  //  rnu::vec2i desired_size() const {
  //    return _text.size();
  //  }

  //  void on_layout(bool changed, rnu::box<2, int> bounds) override
  //  {
  //    frame_base::on_layout(changed, bounds);
  //    if (changed)
  //    {
  //      _draw_position = bounds.position;
  //      _draw_size = bounds.size;
  //    }
  //  }
  //  void on_render(draw_state_base& state, render_target& target) override
  //  {
  //    frame_base::on_render(state, target);
  //    _text.draw(state, _draw_position.x, _draw_position.y, _draw_size.x, _draw_size.y);
  //  }

  //private:
  //  rnu::vec2 _draw_position;
  //  rnu::vec2 _draw_size;
  //  text _text;
  //};

  class group_base : public frame_base
  {
  public:
    using frame_base::frame_base;

    struct layout_data
    {
      item_justify justify_horizontal = item_justify::start;
      item_justify justify_vertical = item_justify::start;
    };

    void add_child(handle_ref<element_base> child)
    {
      _children.push_back(std::move(child));
      invalidate_layout();
    }

    bool needs_relayout() const override
    {
      if (frame_base::needs_relayout())
        return true;

      for (auto const& c : _children)
        if (c.get().needs_relayout())
          return true;

      return false;
    }

    bool needs_redraw() const override
    {
      if (frame_base::needs_redraw())
        return true;

      for (auto const& c : _children)
        if (c.get().needs_redraw())
          return true;

      return false;
    }

  protected:
    rnu::vec2i on_measure(layout_hint x_hint, layout_hint y_hint) override;
    void on_layout(bool changed, rnu::box<2, int> bounds) override;
    void on_render(draw_state_base& state, render_target& target) override;

    void measure_children()
    {
      for (auto& c : _children)
      {
        std::optional<layout_hint> h_hint;
        std::optional<layout_hint> w_hint;

        rnu::vec2i const margins_top_left { c.get().margins().x, c.get().margins().y };
        rnu::vec2i const margins_bottom_right { c.get().margins().z, c.get().margins().w };

        std::visit(rnu::overload{
          [&](int w) { w_hint = layout_exact(w); },
          [&](layout_mode mode) {
            switch (mode)
            {
            case layout_mode::match_parent:
              w_hint = layout_exact(measured_size().x - margins_top_left.x - margins_bottom_right.x);
              break;
            }
          }
          }, c.get().get_layout_params().width);
        std::visit(rnu::overload{
          [&](int h) { h_hint = layout_exact(h); },
          [&](layout_mode mode) {
            switch (mode)
            {
            case layout_mode::match_parent:
              h_hint = layout_exact(measured_size().y - margins_top_left.y - margins_bottom_right.y);
              break;
            }
          }
          }, c.get().get_layout_params().height);

        if (!w_hint || !h_hint)
        {
          c.get().measure(w_hint ? w_hint.value() : layout_undefined(measured_size().x), h_hint ? h_hint.value() : layout_undefined(measured_size().y));

          if (!w_hint)
            w_hint = layout_at_most(c.get().measured_size().x);
          if (!h_hint)
            h_hint = layout_at_most(c.get().measured_size().y);
        }

        c.get().measure(w_hint.value(), h_hint.value());
      }
    }

    std::vector<handle_ref<element_base>> const& children() const 
    {
      return _children;
    }
    std::vector<handle_ref<element_base>>& children()
    {
      return _children;
    }

  private:
    std::vector<handle_ref<element_base>> _children;
  };

  rnu::vec2i justify_element(item_justify justify, orientation o, element_base& parent, element_base& child)
  {
    rnu::vec2i position;
    rnu::vec2i shift_mask(o == orientation::horizontal, o == orientation::vertical);
    rnu::vec2i justify_mask(o != orientation::horizontal, o != orientation::vertical);
    rnu::vec2i placement;
    switch (justify)
    {
    case item_justify::start:
      placement = { 0, 0 };
      break;
    case item_justify::middle:
      placement = (justify_mask * (parent.measured_size() / 2 - child.measured_size_with_margins() / 2));
      break;
    case item_justify::end:
      placement = (justify_mask * (parent.measured_size() - child.measured_size_with_margins()));
      break;
    }
    return placement;
  }

  rnu::vec2i group_base::on_measure(layout_hint x_hint, layout_hint y_hint)
  {
    frame_base::on_measure(x_hint, y_hint);
    measure_children(); // todo: now add wrap_content handling?

    int height = 0;
    int width = 0;
    for (auto& c : children())
    {
      width = std::max(width, c.get().measured_size_with_margins().x);
      height = std::max(height, c.get().measured_size_with_margins().y);
    }

    return frame_base::on_measure(x_hint.second == stretch_mode::exact ? x_hint : layout_exact(width),
      y_hint.second == stretch_mode::exact ? y_hint : layout_exact(height));
  }

  void group_base::on_layout(bool changed, rnu::box<2, int> bounds)
  {
    frame_base::on_layout(changed, bounds);

    if (changed || needs_relayout())
    {
      measure_children();
      
      for (auto& c : _children)
      {
        layout_data layout{};
        if (auto const* params = std::any_cast<layout_data>(&c.get().get_layout_params().additional_data))
          layout = *params;

        auto const offset_x = justify_element(layout.justify_horizontal, orientation::vertical, *this,
          c.get()).x;
        auto const offset_y = justify_element(layout.justify_vertical, orientation::horizontal, *this,
          c.get()).y;

        c.get().layout({ bounds.position + rnu::vec2i{offset_x, offset_y}, c.get().measured_size() });
      }
    }
  }

  void group_base::on_render(draw_state_base& state, render_target& target)
  {
    frame_base::on_render(state, target);

    for (auto& c : _children)
      c.get().render(state, target);
  }

  class stack_base : public group_base
  {
  public:
    using group_base::group_base;

    struct layout_data
    {
      item_justify justify = item_justify::start;
      float weight = 0.0f;
    };

    void set_orientation(orientation o)
    {
      _orientation = o;
      invalidate_layout();
    }

    void justify_content(item_justify j)
    {
      _justify = j;
      invalidate_layout();
    }

    rnu::vec2i on_measure(layout_hint x_hint, layout_hint y_hint) override
    {
      frame_base::on_measure(x_hint, y_hint);
      measure_children();

      int height = 0;
      int width = 0;

      float total_weights = 0.0f;
      for (auto& c : children())
      {
        if (auto const* params = std::any_cast<layout_data>(&c.get().get_layout_params().additional_data))
        {
          total_weights += params->weight;
        }
      }

      for (auto& c : children())
      {
        bool has_weight = false;
        if (auto const* params = std::any_cast<layout_data>(&c.get().get_layout_params().additional_data))
        {
          if (params->weight != 0.0f)
            has_weight = true;
        }

        if (_orientation == orientation::horizontal)
        {
          if(!has_weight)
            width += c.get().measured_size_with_margins().x;
          height = std::max(height, c.get().measured_size_with_margins().y);
        }
        else
        {
          width = std::max(width, c.get().measured_size_with_margins().x);
          if (!has_weight)
            height += c.get().measured_size_with_margins().y;
        }
      }

      auto const measured = frame_base::on_measure(x_hint.second == stretch_mode::exact ? x_hint : layout_exact(width),
        y_hint.second == stretch_mode::exact ? y_hint : layout_exact(height));

      // now lay out weights
      for (auto& c : children())
      {
        if (auto const* params = std::any_cast<layout_data>(&c.get().get_layout_params().additional_data))
        {
          if (params->weight != 0.0f)
          {
            auto const factor = params->weight / total_weights;
            auto size = c.get().measured_size();
            if (_orientation == orientation::horizontal)
            {
              // x
              size.x = (measured.x - width) * factor - c.get().margins().x - c.get().margins().z;
            }
            else
            {
              // y
              size.y = (measured.y - width) * factor - c.get().margins().y - c.get().margins().w;
            }
            c.get().measure(layout_exact(size.x), layout_exact(size.y));
          }
        }
      }

      return measured;
    }

    void on_layout(bool changed, rnu::box<2, int> bounds) override
    {
      frame_base::on_layout(changed, bounds);

      if (changed || needs_relayout())
      {
        //measure_children();

        rnu::vec2i position;
        rnu::vec2i shift_mask(_orientation == orientation::horizontal, _orientation == orientation::vertical);
        rnu::vec2i justify_mask(_orientation != orientation::horizontal, _orientation != orientation::vertical);
        for (auto& c : children())
        {
          item_justify justify = _justify;
          if (auto const* params = std::any_cast<layout_data>(&c.get().get_layout_params().additional_data))
            justify = params->justify;

          auto const offset = justify_element(justify, _orientation, *this,
            c.get());

          c.get().layout({ bounds.position + position + offset, c.get().measured_size() });
          position += shift_mask * c.get().measured_size_with_margins();
        }
      }
    }

  protected:
    orientation _orientation = orientation::vertical;
    item_justify _justify = item_justify::middle;
  };

  //using label = handle<text_base>;
  using group = handle<group_base>;
  using stack = handle<stack_base>;
}
































































int main()
{

#if 0
  goop::font fnt(res / "MPLUS1p - Regular.ttf");


  std::vector<std::uint8_t> atlas_img;
  int aiw, aih;
  atlas.dump(atlas_img, aiw, aih);

  stbi_write_png("../../../../../xXfontatlasXx.png", aiw, aih, 1, atlas_img.data(), 0);

  std::vector<std::uint8_t> out_image(1024 * 512);

  auto const set_text = atlas.text_set(L"Gefiewaltige Krater benützt man nötigst. 50% Maßschneiderung!", 18, rnu::vec2{ 33, 33 });
  for (auto const& set : set_text)
  {
    auto dst = set.bounds;
    dst.position /= rnu::vec2(1024, 512);
    dst.size /= rnu::vec2(1024, 512);

    copy_linear<std::uint8_t>(out_image, 1024, 512, dst, atlas_img, aiw, aih, set.uvs);
  }

  stbi_write_png("../../../../../PUPI.png", 1024, 512, 1, out_image.data(), 0);

  auto const load_letter = [&](std::optional<goop::glyph_id> ch) -> std::vector<goop::lines::line> const& {
    static std::mutex m;
    std::unique_lock lock(m);

    static thread_local std::vector<goop::outline_segment> outline;
    outline.clear();
    static thread_local std::vector<goop::lines::line> letter;
    letter.clear();

    rnu::rect2f bounds;
    auto const gly = *ch;
    struct
    {
      void operator()(goop::line const& line)
      {
        goop::lines::subsample(goop::lines::line{
          .start = line.start,
          .end = line.end
          }, 3, letter);
      }
      void operator()(goop::bezier const& line)
      {
        goop::lines::subsample(goop::lines::bezier{
          .start = line.start,
          .control = line.control,
          .end = line.end
          }, 3, letter);
      }
    } visitor;

    fnt.outline(gly, bounds, [&](goop::outline_segment const& o) { std::visit(visitor, o); });

    return letter;
  };

  auto const load_image = [](auto&& file, int& x, int& y)
  {
    int ch;
    auto* data = stbi_load(std::data(file), &x, &y, &ch, 3);
    std::vector<stbi_uc> result(data, data + x * y * 3);
    stbi_image_free(data);
    return result;
  };

  auto const load_svg = [](auto&& path) {
    goop::vector_image img;
    img.parse(path);
    return goop::lines::to_line_segments(img) /*subsampled_shape(img, 3)*/;
  };

  struct vector_icon
  {
    std::string path;
    float scale;
  };

  struct glyph
  {
    goop::glyph_id id;
    float scale;
  };

  struct raster_image
  {
    std::filesystem::path path;
    const float scale = 1;
  };

  using vector_path = std::variant<glyph, vector_icon, raster_image>;

  {
    auto const size = 20;
    auto const scale = size / fnt.units_per_em();
    auto const sdf_radius = 2.f;

    rnu::thread_pool pool;

    std::vector<rnu::rect2f> bounds;
    std::vector<vector_path> paths;

    int const pad = std::ceilf(sdf_radius);
    auto const add = [&](rnu::rect2f r, auto type)
    {
      if constexpr (!std::same_as<std::decay_t<decltype(type)>, raster_image>)
      {
        r.size *= type.scale;
        r.size += 2 * pad;
        r.position *= type.scale;
      }
      bounds.push_back(std::move(r));
      paths.push_back(std::move(type));
    };

    add({ {0,0},{24,24} }, vector_icon{ "M12,21.35L10.55,20.03C5.4,15.36 2,12.27 2,8.5C2,5.41 4.42,3 7.5,3C9.24,3 10.91,3.81 12,5.08C13.09,3.81 14.76,3 16.5,3C19.58,3 22,5.41 22,8.5C22,12.27 18.6,15.36 13.45,20.03L12,21.35Z", 1.0f });
    add({ {0,0},{24,24} }, vector_icon{ "M12.1,18.55L12,18.65L11.89,18.55C7.14,14.24 4,11.39 4,8.5C4,6.5 5.5,5 7.5,5C9.04,5 10.54,6 11.07,7.36H12.93C13.46,6 14.96,5 16.5,5C18.5,5 20,6.5 20,8.5C20,11.39 16.86,14.24 12.1,18.55M16.5,3C14.76,3 13.09,3.81 12,5.08C10.91,3.81 9.24,3 7.5,3C4.42,3 2,5.41 2,8.5C2,12.27 5.4,15.36 10.55,20.03L12,21.35L13.45,20.03C18.6,15.36 22,12.27 22,8.5C22,5.41 19.58,3 16.5,3Z", 1.0f });
    add({ {0,0},{24,24} }, vector_icon{ "M8,5.14V19.14L19,12.14L8,5.14Z", 2.0f });
    add({ {0,0},{24,24} }, vector_icon{ "M9,5A4,4 0 0,1 13,9A4,4 0 0,1 9,13A4,4 0 0,1 5,9A4,4 0 0,1 9,5M9,15C11.67,15 17,16.34 17,19V21H1V19C1,16.34 6.33,15 9,15M16.76,5.36C18.78,7.56 18.78,10.61 16.76,12.63L15.08,10.94C15.92,9.76 15.92,8.23 15.08,7.05L16.76,5.36M20.07,2C24,6.05 23.97,12.11 20.07,16L18.44,14.37C21.21,11.19 21.21,6.65 18.44,3.63L20.07,2Z", 1.0f });
    add({ {0,0},{24,24} }, vector_icon{ "M22.11 21.46L2.39 1.73L1.11 3L5.2 7.09C3.25 7.5 1.85 9.27 2 11.31C2.12 12.62 2.86 13.79 4 14.45V16C4 16.55 4.45 17 5 17H7V14.88C5.72 13.58 5 11.83 5 10C5 9.11 5.18 8.23 5.5 7.4L7.12 9C6.74 10.84 7.4 12.8 9 14V16C9 16.55 9.45 17 10 17H14C14.31 17 14.57 16.86 14.75 16.64L17 18.89V19C17 19.34 16.94 19.68 16.83 20H18C18.03 20 18.06 20 18.09 20L20.84 22.73L22.11 21.46M9.23 11.12L10.87 12.76C10.11 12.46 9.53 11.86 9.23 11.12M13 15H11V12.89L13 14.89V15M10.57 7.37L9.13 5.93C10.86 4.72 13.22 4.67 15 6C16.26 6.94 17 8.43 17 10C17 11.05 16.67 12.05 16.08 12.88L14.63 11.43C14.86 11 15 10.5 15 10C15 8.34 13.67 7 12 7C11.5 7 11 7.14 10.57 7.37M17.5 14.31C18.47 13.09 19 11.57 19 10C19 8.96 18.77 7.94 18.32 7C19.63 7.11 20.8 7.85 21.46 9C22.57 10.9 21.91 13.34 20 14.45V16C20 16.22 19.91 16.42 19.79 16.59L17.5 14.31M10 18H14V19C14 19.55 13.55 20 13 20H11C10.45 20 10 19.55 10 19V18M7 19C7 19.34 7.06 19.68 7.17 20H6C5.45 20 5 19.55 5 19V18H7V19Z", 1.0f });

    int iiix, iiiy, iiich;
    stbi_info("../../../../../res/vintagething.jpg", &iiix, &iiiy, &iiich);
    add(rnu::rect2f{ {0,0}, {iiix, iiiy} }, raster_image{ "../../../../../res/vintagething.jpg" });
    stbi_info("../../../../../res/testsmall.jpg", &iiix, &iiiy, &iiich);
    add(rnu::rect2f{ {0,0}, {iiix, iiiy} }, raster_image{ "../../../../../res/testsmall.jpg" });

    const std::pair<char16_t, char16_t> char_ranges[] = {
      goop::character_ranges::basic_latin,
      goop::character_ranges::c1_controls_and_latin_1_supplement,
    };

    for (auto p : char_ranges)
    {
      for (char16_t x = p.first; x <= p.second; ++x)
      {
        auto const gly = fnt.glyph(x);
        if (gly == goop::glyph_id::missing)
          continue;

        add(fnt.get_rect(gly), glyph{ gly, scale });
      }
    }

    std::vector<rnu::rect2f> original_bounds = bounds;


    int iw = 1024;
    goop::skyline_packer skyline;
    int ih = skyline.pack(bounds, iw);

    std::vector<rnu::vec3ui8> img(iw * ih);

    auto const conc = pool.concurrency();
    std::vector<std::future<void>> futs(conc);

    for (int n = 0; n < conc; ++n)
    {
      futs[n] = pool.run_async([&, n] {
        for (int i = n; i < bounds.size(); i += conc)
        {
          std::visit(rnu::overload{
            [&](glyph g) {
              emplace(
                load_letter(g.id),
                bounds[i],
                g.scale,
                0, sdf_radius, img, iw, ih, 0, 0,
                -bounds[i].position + original_bounds[i].position - rnu::vec2(pad, pad));
            },
            [&](vector_icon icon) {
              emplace(
                load_svg(icon.path),
                bounds[i],
                icon.scale,
                0, sdf_radius, img, iw, ih, 0, 0,
                -bounds[i].position + original_bounds[i].position - rnu::vec2(pad, pad));
            },
            [&](raster_image image)
            {
              int w, h;
              auto const data = load_image(image.path.string(), w, h);
              for (int x = 0; x < w; ++x)
              {
                for (int y = 0; y < h; ++y)
                {
                  int const ix = x + bounds[i].position.x;
                  int const iy = y + bounds[i].position.y;
                  memcpy(&img[ix + iy * iw], &data[(x + y * w) * 3], 3);
                }
              }
            }
            }, paths[i]);


        }
        });
    }

    rnu::await_all(futs);

    stbi_write_png("../../../../../result.png", iw, ih, 3, img.data(), 0);
  }

#endif

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

  rnu::shared_entity ptcl = load_gltf_as_entity(res / "ptcl/scene.gltf", ecs, app, main_geometry);
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
    .diffuse_map = tex((res / "grass.jpg").string().c_str()),
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
    rnu::shared_entity& ball = balls.emplace_back(load_gltf_as_entity(res / "ballymcballface/scene.gltf", ecs, app, main_geometry));
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
  goop::shader vert(goop::shader_type::vertex, model_shaders::model_vs(), &shader_log);
  std::cout << "VSLOG: " << shader_log << '\n';
  goop::shader frag(goop::shader_type::fragment, model_shaders::model_fs(), &shader_log);
  std::cout << "FSLOG: " << shader_log << '\n';
  pipeline->use(vert);
  pipeline->use(frag);
  goop::mapped_buffer<matrices> matrix_buffer{ 1 };

  int img = 0;


  //goop::vector_image const vectors[] = {
  //    goop::vector_image{ "M12,21.35L10.55,20.03C5.4,15.36 2,12.27 2,8.5C2,5.41 4.42,3 7.5,3C9.24,3 10.91,3.81 12,5.08C13.09,3.81 14.76,3 16.5,3C19.58,3 22,5.41 22,8.5C22,12.27 18.6,15.36 13.45,20.03L12,21.35Z", 0, 0, 24, 24 },
  //    goop::vector_image{ "M12.1,18.55L12,18.65L11.89,18.55C7.14,14.24 4,11.39 4,8.5C4,6.5 5.5,5 7.5,5C9.04,5 10.54,6 11.07,7.36H12.93C13.46,6 14.96,5 16.5,5C18.5,5 20,6.5 20,8.5C20,11.39 16.86,14.24 12.1,18.55M16.5,3C14.76,3 13.09,3.81 12,5.08C10.91,3.81 9.24,3 7.5,3C4.42,3 2,5.41 2,8.5C2,12.27 5.4,15.36 10.55,20.03L12,21.35L13.45,20.03C18.6,15.36 22,12.27 22,8.5C22,5.41 19.58,3 16.5,3Z", 0, 0, 24, 24 },
  //    goop::vector_image{ "M8,5.14V19.14L19,12.14L8,5.14Z", 0, 0, 24, 24 },
  //    goop::vector_image{ "M9,5A4,4 0 0,1 13,9A4,4 0 0,1 9,13A4,4 0 0,1 5,9A4,4 0 0,1 9,5M9,15C11.67,15 17,16.34 17,19V21H1V19C1,16.34 6.33,15 9,15M16.76,5.36C18.78,7.56 18.78,10.61 16.76,12.63L15.08,10.94C15.92,9.76 15.92,8.23 15.08,7.05L16.76,5.36M20.07,2C24,6.05 23.97,12.11 20.07,16L18.44,14.37C21.21,11.19 21.21,6.65 18.44,3.63L20.07,2Z", 0, 0, 24, 24 },
  //    goop::vector_image{ "M22.11 21.46L2.39 1.73L1.11 3L5.2 7.09C3.25 7.5 1.85 9.27 2 11.31C2.12 12.62 2.86 13.79 4 14.45V16C4 16.55 4.45 17 5 17H7V14.88C5.72 13.58 5 11.83 5 10C5 9.11 5.18 8.23 5.5 7.4L7.12 9C6.74 10.84 7.4 12.8 9 14V16C9 16.55 9.45 17 10 17H14C14.31 17 14.57 16.86 14.75 16.64L17 18.89V19C17 19.34 16.94 19.68 16.83 20H18C18.03 20 18.06 20 18.09 20L20.84 22.73L22.11 21.46M9.23 11.12L10.87 12.76C10.11 12.46 9.53 11.86 9.23 11.12M13 15H11V12.89L13 14.89V15M10.57 7.37L9.13 5.93C10.86 4.72 13.22 4.67 15 6C16.26 6.94 17 8.43 17 10C17 11.05 16.67 12.05 16.08 12.88L14.63 11.43C14.86 11 15 10.5 15 10C15 8.34 13.67 7 12 7C11.5 7 11 7.14 10.57 7.37M17.5 14.31C18.47 13.09 19 11.57 19 10C19 8.96 18.77 7.94 18.32 7C19.63 7.11 20.8 7.85 21.46 9C22.57 10.9 21.91 13.34 20 14.45V16C20 16.22 19.91 16.42 19.79 16.59L17.5 14.31M10 18H14V19C14 19.55 13.55 20 13 20H11C10.45 20 10 19.55 10 19V18M7 19C7 19.34 7.06 19.68 7.17 20H6C5.45 20 5 19.55 5 19V18H7V19Z", 0, 0, 24, 24 }
  //};
  //goop::vector_graphics_holder holder(512, 25, vectors);

  goop::gui::sdf_font font(1024, 25, 8, goop::font(res / "SawarabiGothic-Regular.ttf"), std::array{
    goop::character_ranges::basic_latin,
 /*   goop::character_ranges::c1_controls_and_latin_1_supplement,
    goop::character_ranges::latin_extended_a,
    goop::character_ranges::latin_extended_additional,
    goop::character_ranges::latin_extended_b,
    goop::character_ranges::alphabetic_presentation_forms,
    goop::character_ranges::currency_symbols,*/
    /*goop::character_ranges::katakana,
    goop::character_ranges::katakana_phonetic_extensions,
    goop::character_ranges::hiragana,
    goop::character_ranges::make_char_range(0x79D2, 0x79D2),*/
    });


  //goop::gui::symbol my_symbol;
  //my_symbol.set_holder(holder);
  //my_symbol.set_icon(4);
  //my_symbol.set_scale(0.5);
  //my_symbol.set_color({ 0,0,0, 1 });

  //goop::gui::text fps_text;
  //fps_text.set_font(atlas);
  //fps_text.set_size(10);
  //fps_text.set_color({ 0, 0, 0, 1});

  //goop::gui::text title_text;
  //title_text.set_font(atlas);
  //title_text.set_size(25);
  //title_text.set_text(L"GOOP Graphics Whatever");
  //title_text.set_color({ 1,1,1, 1 });
  //title_text.set_border_color({ 0,0,0,1 });
  //title_text.set_border_width(1.5);
  //title_text.set_border_offset(0.5);
  //title_text.set_outer_smoothness(2);


  //goop::gui::panel_2d panel;
  //panel.set_color({ 1,1,1,1.0 });
  //panel.set_texture(tex("../../../../../res/pattern.jpg"));
  
  //goop::vector_image const vectors[] = {
  //    goop::vector_image{ "M12,21.35L10.55,20.03C5.4,15.36 2,12.27 2,8.5C2,5.41 4.42,3 7.5,3C9.24,3 10.91,3.81 12,5.08C13.09,3.81 14.76,3 16.5,3C19.58,3 22,5.41 22,8.5C22,12.27 18.6,15.36 13.45,20.03L12,21.35Z", 0, 0, 24, 24 },
  //    goop::vector_image{ "M12.1,18.55L12,18.65L11.89,18.55C7.14,14.24 4,11.39 4,8.5C4,6.5 5.5,5 7.5,5C9.04,5 10.54,6 11.07,7.36H12.93C13.46,6 14.96,5 16.5,5C18.5,5 20,6.5 20,8.5C20,11.39 16.86,14.24 12.1,18.55M16.5,3C14.76,3 13.09,3.81 12,5.08C10.91,3.81 9.24,3 7.5,3C4.42,3 2,5.41 2,8.5C2,12.27 5.4,15.36 10.55,20.03L12,21.35L13.45,20.03C18.6,15.36 22,12.27 22,8.5C22,5.41 19.58,3 16.5,3Z", 0, 0, 24, 24 },
  //    goop::vector_image{ "M8,5.14V19.14L19,12.14L8,5.14Z", 0, 0, 24, 24 },
  //    goop::vector_image{ "M9,5A4,4 0 0,1 13,9A4,4 0 0,1 9,13A4,4 0 0,1 5,9A4,4 0 0,1 9,5M9,15C11.67,15 17,16.34 17,19V21H1V19C1,16.34 6.33,15 9,15M16.76,5.36C18.78,7.56 18.78,10.61 16.76,12.63L15.08,10.94C15.92,9.76 15.92,8.23 15.08,7.05L16.76,5.36M20.07,2C24,6.05 23.97,12.11 20.07,16L18.44,14.37C21.21,11.19 21.21,6.65 18.44,3.63L20.07,2Z", 0, 0, 24, 24 },
  //    goop::vector_image{ "M22.11 21.46L2.39 1.73L1.11 3L5.2 7.09C3.25 7.5 1.85 9.27 2 11.31C2.12 12.62 2.86 13.79 4 14.45V16C4 16.55 4.45 17 5 17H7V14.88C5.72 13.58 5 11.83 5 10C5 9.11 5.18 8.23 5.5 7.4L7.12 9C6.74 10.84 7.4 12.8 9 14V16C9 16.55 9.45 17 10 17H14C14.31 17 14.57 16.86 14.75 16.64L17 18.89V19C17 19.34 16.94 19.68 16.83 20H18C18.03 20 18.06 20 18.09 20L20.84 22.73L22.11 21.46M9.23 11.12L10.87 12.76C10.11 12.46 9.53 11.86 9.23 11.12M13 15H11V12.89L13 14.89V15M10.57 7.37L9.13 5.93C10.86 4.72 13.22 4.67 15 6C16.26 6.94 17 8.43 17 10C17 11.05 16.67 12.05 16.08 12.88L14.63 11.43C14.86 11 15 10.5 15 10C15 8.34 13.67 7 12 7C11.5 7 11 7.14 10.57 7.37M17.5 14.31C18.47 13.09 19 11.57 19 10C19 8.96 18.77 7.94 18.32 7C19.63 7.11 20.8 7.85 21.46 9C22.57 10.9 21.91 13.34 20 14.45V16C20 16.22 19.91 16.42 19.79 16.59L17.5 14.31M10 18H14V19C14 19.55 13.55 20 13 20H11C10.45 20 10 19.55 10 19V18M7 19C7 19.34 7.06 19.68 7.17 20H6C5.45 20 5 19.55 5 19V18H7V19Z", 0, 0, 24, 24 }
  //};
  //goop::vector_graphics_holder holder(512, 25, vectors);

  //goop::gui::sdf_font atlas(1024, 25, 8, goop::font("../../../../../res/SawarabiGothic-Regular.ttf"), std::array{
  //  goop::character_ranges::basic_latin,
  //  goop::character_ranges::c1_controls_and_latin_1_supplement,
  //  goop::character_ranges::latin_extended_a,
  //  goop::character_ranges::latin_extended_additional,
  //  goop::character_ranges::latin_extended_b,
  //  goop::character_ranges::alphabetic_presentation_forms,
  //  goop::character_ranges::currency_symbols,
  //  /*goop::character_ranges::katakana,
  //  goop::character_ranges::katakana_phonetic_extensions,
  //  goop::character_ranges::hiragana,
  //  goop::character_ranges::make_char_range(0x79D2, 0x79D2),*/
  //  });


  //goop::gui::symbol my_symbol;
  //my_symbol.set_holder(holder);
  //my_symbol.set_icon(4);
  //my_symbol.set_scale(0.5);
  //my_symbol.set_color({ 0,0,0, 1 });

  //goop::gui::text fps_text;
  //fps_text.set_font(atlas);
  //fps_text.set_size(10);
  //fps_text.set_color({ 0, 0, 0, 1});

  //goop::gui::text title_text;
  //title_text.set_font(atlas);
  //title_text.set_size(25);
  //title_text.set_text(L"GOOP Graphics Whatever");
  //title_text.set_color({ 1,1,1, 1 });
  //title_text.set_border_color({ 0,0,0,1 });
  //title_text.set_border_width(1.5);
  //title_text.set_border_offset(0.5);
  //title_text.set_outer_smoothness(2);


  //goop::gui::panel_2d panel;
  //panel.set_color({ 1,1,1,1.0 });
  //panel.set_texture(tex("../../../../../res/pattern.jpg"));


  goop::ui::ui_context context(app.default_texture_provider());

  namespace el = goop::gui::el;
  el::group root_frame(context);

  auto const mkl = [&](auto const text)
  {
    el::label lbl(context);
    lbl->set_font(font);
    lbl->set_color(rnu::vec4(1, 1, 1, 0.8));
    lbl->set_text(text);
    lbl->set_size(10);
    lbl->set_layout_params(el::layout_params{
      .width = el::layout_mode::wrap_content,
      .height = el::layout_mode::wrap_content
      });
    return lbl;
  };
  el::label lbl(context);
  lbl->set_font(font);
  lbl->set_color(rnu::vec4(1, 1, 1, 1));
  lbl->set_text(L"Abacus totalis 500");
  lbl->set_size(14);
  lbl->set_layout_params(el::to_layout("wrap_content"));

  el::stack lbl_frame(context); 
  lbl_frame->set_orientation(el::orientation::vertical);
  lbl_frame->set_layout_params(el::layout_params{
    .width = el::layout_mode::wrap_content,
    .height = el::layout_mode::wrap_content
    });
  lbl_frame->justify_content(el::item_justify::start);
  lbl_frame->add_child(mkl(L"Framerate:"));
  lbl_frame->add_child(lbl);
  lbl_frame->set_margins({ 8 });

  el::group lbl_backdrop(context);
  lbl_backdrop->set_layout_params(el::to_layout("wrap_content", el::stack_base::layout_data{
    .justify = el::item_justify::middle,
    .weight = 1.0f
    }));
  lbl_backdrop->set_background(rnu::vec4(0,0,0, 0.4));
  lbl_backdrop->add_child(lbl_frame);
  lbl_backdrop->set_margins({ 2, 2, 2, 2 });

  el::group lbl_backdrop2(context);
  lbl_backdrop2->set_layout_params(el::to_layout("match_parent, match_parent", el::stack_base::layout_data{
    .justify = el::item_justify::middle,
    .weight = 5.0f
    }));
  lbl_backdrop2->set_background(rnu::vec4(0, 0, 0, 0.5));
  lbl_backdrop2->set_margins({ 0, 2, 2, 2 });


  el::frame image(context);
  image->set_background(tex((res / "profile.png").string().c_str()));
  image->set_layout_params(el::layout_params{ 32, el::layout_mode::match_parent });
  image->set_background_scaling(el::frame_base::background_scaling::stretch_fit);
  image->set_margins({ 8,8,6,8 });

  el::stack text_and_image(context);
  text_and_image->add_child(image);
  text_and_image->add_child(lbl_backdrop);
  text_and_image->add_child(lbl_backdrop2);
  text_and_image->set_orientation(el::orientation::horizontal);
  text_and_image->set_background(rnu::vec4(0.2,0.2,0.25,1.0));
  text_and_image->set_margins({ 2 });
  text_and_image->set_layout_params(el::layout_params{ el::layout_mode::match_parent,
    el::layout_mode::wrap_content});

  el::group text_and_image_outline(context);
  text_and_image_outline->set_background(rnu::vec4(0,0,0, 0.9f));
  text_and_image_outline->set_margins({ 4 });
  text_and_image_outline->add_child(text_and_image);
  text_and_image_outline->set_layout_params(el::layout_params{ el::layout_mode::match_parent,
    el::layout_mode::wrap_content,
    el::group_base::layout_data{
      el::item_justify::end,
      el::item_justify::end
    } });

  root_frame->add_child(text_and_image_outline);

















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
    state->set_viewport({ {0,0}, {window_width, window_height} });
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

    static auto nX = 0.0;
    static auto fn = 0;

    nX += app.current_delta_time().count();
    fn += 1;

    if (nX > 0.6)
    {
      //fps_text.set_text(L"フレームレートは" + std::to_wstring(std::int32_t(fn / nX)) + L"フレーム/秒です(Ja natürlich, viel Spaß!)");
      //fps_text.set_text(L"Framerate: " + std::to_wstring(std::int32_t(fn / nX)) + L"fps (Ja natürlich, viel Spaß!)");
      lbl->set_text(std::to_wstring(std::int32_t(fn / nX)) + L" fps");
      nX = 0.0;
      fn = 0;
    }

    //rnu::vec2 const ttp(window_width / 2.f - title_text.size().x / 2.f, window_height - title_text.size().y);
    //panel.set_position(ttp);
    //panel.set_size(title_text.size());
    //panel.set_uv({ 0, 0 }, rnu::vec2{ title_text.size().x / title_text.size().y, 1 } / 3);
    //panel.draw(state);
    //my_symbol.draw(state, 0, 0);
    //title_text.draw(state, ttp.x, ttp.y);
    //fps_text.draw(state, 30, 0);

    el::draw_root(root_frame, state, app.default_render_target(), window_width, window_height);

    img = app.end_frame();
  }
}
