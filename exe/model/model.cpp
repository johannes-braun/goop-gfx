#include <default_app.hpp>
#include <file/obj.hpp>
#include <iostream>
#include <variant>
#include <experimental/generator>
#include <animation/animation.hpp>
#include <file/gltf__WIP.hpp>
#include <rnu/ecs/ecs.hpp>
#include <rnu/ecs/component.hpp>

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
  pass_normal = vec3(inverse(transpose(mat4(mat3(skinMat)))) * vec4(normal, 0));
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

  vec4 tex_color = (has_texture != 0) ? mat_color * texture(image_texture, ux) : mat_color;
  
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

struct transform_component : rnu::component<transform_component>
{
  rnu::mat4 transform;
};

struct rigging_component : rnu::component<rigging_component>
{
  rnu::weak_entity skeleton;
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
    add_component_type<transform_component>(rnu::component_flag::optional);
  }

  virtual void update(duration_type delta, rnu::component_base** components) const
  {
    auto* rig = static_cast<rigging_skeleton*>(components[0]);
    auto* transform = static_cast<transform_component*>(components[1]);

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
    add_component_type<transform_component>();
    add_component_type<material_component>();
    add_component_type<geometry_component>();
    add_component_type<rigging_component>(rnu::component_flag::optional);
  }

  void set_draw_state(goop::draw_state& state)
  {
    _draw_state = &state;
  }

  virtual void update(duration_type delta, rnu::component_base** components) const
  {
    auto* transform = static_cast<transform_component*>(components[0]);
    auto* material = static_cast<material_component*>(components[1]);
    auto* geometry = static_cast<geometry_component*>(components[2]);
    auto* rig = static_cast<rigging_component*>(components[3]);

    rigging_skeleton* skeleton = !rig ? nullptr :
      rig->skeleton.lock()->get<rigging_skeleton>();
    transform_component* skeleton_tf = !rig ? nullptr :
      rig->skeleton.lock()->get<transform_component>();

    if (skeleton)
      skeleton->joints->bind(*_draw_state, 1);

    rnu::mat4 model_mat = transform->transform;
    if (skeleton_tf)
      model_mat = skeleton_tf->transform * model_mat;

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
    rigging_skeleton{
      .nodes = gltf.nodes,
      .skin = gltf.skins[0],
      .animations = gltf.animations,
      .active_animation = ""
    },
    grouped_entity_component{
      .name = "Rig"
    }
  );
  auto* rig = skeleton->get<rigging_skeleton>();
  auto* group = skeleton->get<grouped_entity_component>();

  for (size_t i = 0; i < gltf.materials.size(); ++i)
  {
    auto& geoms = gltf.material_geometries[i];
    group->entities.push_back(
      ecs.create_entity_shared(
        geometry_component{ .geometry = main_geometry, .parts = geoms },
        material_component{ .material = gltf.materials[i] },
        transform_component{ .transform = rnu::mat4{1.0f} }
      )
    );
    if (!gltf.animations.empty())
      group->entities.back()->add(rigging_component{ .skeleton = skeleton });
  }
  return skeleton;
}

int main()
{
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

  rnu::system_list graphics_list;
  graphics_list.add(rigging_system);
  graphics_list.add(object_renderer);

  goop::geometry main_geometry;

  rnu::shared_entity ptcl = load_gltf_as_entity("../../../../../res/ptcl/scene.gltf", ecs, app, main_geometry);
  rnu::shared_entity mrr = load_gltf_as_entity("../../../../../res/mrr/scene.gltf", ecs, app, main_geometry);
  rnu::shared_entity wolf = load_gltf_as_entity("../../../../../res/wolf/scene.gltf", ecs, app, main_geometry);

  wolf->add(transform_component{ .transform = rnu::scale(rnu::vec3(4.0f)) });
  wolf->get<rigging_skeleton>()->run_animation("Take 001");

  ptcl->add(transform_component{ .transform = rnu::scale(rnu::vec3(2.f)) });
  ptcl->get<rigging_skeleton>()->run_animation("Idle");

  mrr->add(transform_component{ .transform = rnu::scale(rnu::vec3(0.05f)) });
  mrr->get<rigging_skeleton>()->run_animation("_arm|_arm|_arm|_arm|_arm|motion marie rose_bone|_arm|motion marie ro");

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
      ptcl->get<rigging_skeleton>()->run_animation("Running");
      break;
    case goop::input_key::state::release:
      ptcl->get<rigging_skeleton>()->run_animation("Idle");
      break;
    }

    sampler->bind(state, 0);
    object_renderer.set_draw_state(app.default_draw_state());
    ecs.update(app.current_delta_time(), graphics_list);
    img = app.end_frame();
  }
}