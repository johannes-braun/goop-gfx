#include <default_app.hpp>
#include <file/obj.hpp>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_gltf.h>
#include <iostream>
#include <variant>
#include <experimental/generator>

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

layout(std430, binding = 0) buffer Matrices
{
  mat4 view;
  mat4 proj;
};

layout(std430, binding = 1) buffer Joints
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
  mat4 skinMat = dot(weight_arr, weight_arr) == 0 ? mat4(1.0) :
    (weight_arr.x * joints[int(joint_arr.x)] +
    weight_arr.y * joints[int(joint_arr.y)] +
    weight_arr.z * joints[int(joint_arr.z)] +
    weight_arr.w * joints[int(joint_arr.w)]);

  vec3 pos = position;
  float scale = 2;
  mat4 scaler = mat4(
    scale, 0, 0, 0,
    0, scale, 0, 0,
    0, 0, scale, 0,
    0, 0, 0, 1
  );

  vec4 hom_position = scaler * skinMat * vec4(pos, 1);
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

struct material_association
{
  std::vector<goop::vertex_offset> geometries;
};
struct material
{
  goop::texture diffuse_map;
  struct
  {
    rnu::vec4 color;
    uint32_t has_texture;
  } data;

  goop::mapped_buffer<decltype(data)> data_buffer{1};
  bool dirty = true;

  void bind(goop::draw_state& state) {
    if (dirty)
    {
      dirty = false;
      data_buffer->write(data);
    }
    diffuse_map->bind(state, 0);
    data_buffer->bind(state, 2);
  }
};

struct matrices
{
  rnu::mat4 view;
  rnu::mat4 proj;
};

template<typename T>
struct interpolate_value
{
  T operator()(T const& prev, T const& next, float t) const {
    return prev + (next - prev) * t;
  }
};

template<typename T>
struct interpolate_value<rnu::quat_t<T>>
{
  using quat_type = rnu::quat_t<T>;

  quat_type operator()(quat_type const& prev, quat_type const& next, float t) const {
    return rnu::slerp(prev, next, t);
  }
};

template<typename T>
auto const interp(T a, T b, float t)
{
  return interpolate_value<T>{}(a, b, t);
}

std::vector<size_t> relocations;

struct transform_node
{
  struct loc_rot_scale
  {
    rnu::vec3 location;
    rnu::quat rotation;
    rnu::vec3 scale;
  };
  using transform = std::variant<rnu::mat4, loc_rot_scale>;

  loc_rot_scale& lrs() {
    return std::get<loc_rot_scale>(transformation);
  }
  loc_rot_scale const& lrs() const {
    return std::get<loc_rot_scale>(transformation);
  }

  void set_location(rnu::vec3 location) {
    std::get<loc_rot_scale>(transformation).location = location;
  }
  void set_rotation(rnu::quat rotation) {
    std::get<loc_rot_scale>(transformation).rotation = rotation;
  }
  void set_scale(rnu::vec3 scale) {
    std::get<loc_rot_scale>(transformation).scale = scale;
  }
  void set_matrix(rnu::mat4 matrix) {
    std::get<rnu::mat4>(transformation) = matrix;
  }

  rnu::mat4 matrix() const {
    static constexpr struct
    {
      rnu::mat4 operator()(rnu::mat4 const& mat) const
      {
        return mat;
      }

      rnu::mat4 operator()(loc_rot_scale const& lrs) const
      {
        return rnu::translation(lrs.location) * rnu::rotation(lrs.rotation) * rnu::scale(lrs.scale);
      }
    } visitor{};
    return std::visit(visitor, transformation);
  }

  ptrdiff_t children_offset;
  size_t num_children;
  ptrdiff_t parent;
  transform transformation;
};

enum class animation_target
{
  location,
  rotation,
  scale
};

class animation
{
public:
  using checkpoints = std::variant<
    std::vector<rnu::vec3>,
    std::vector<rnu::quat>
  >;

  animation(animation_target target, size_t node_index, checkpoints checkpoints, std::vector<float> timestamps);

  float duration() const { return _timestamps.empty() ? 0 : _timestamps.back(); }

  void transform(float time, float mix_factor, std::vector<transform_node>& nodes) const {
    if (_timestamps.empty())
      return;

    auto const clamped_time = std::clamp(time, _timestamps.front(), _timestamps.back());

    auto const ts_iter = std::prev(std::upper_bound(begin(_timestamps), end(_timestamps), clamped_time));
    auto const current_ts_index = std::distance(begin(_timestamps), ts_iter);
    auto const next_ts_index = (current_ts_index + 1) % _timestamps.size();

    auto const current_ts = _timestamps[current_ts_index];
    auto const next_ts = _timestamps[next_ts_index];
    auto const i = (next_ts_index == current_ts_index) ? 0.0 : (time - current_ts) / (next_ts - current_ts);

    auto& n = nodes[_node_index];

    switch (_target)
    {
    case animation_target::location:
    {
      auto const& locations = std::get<0>(_checkpoints);
      auto const calc = interp(locations[current_ts_index], locations[next_ts_index], i);
      auto const attenuated = interp(n.lrs().location, calc, mix_factor);

      n.set_location(attenuated);
      break;
    }
    case animation_target::rotation:
    {
      auto const& rotations = std::get<1>(_checkpoints);
      auto const rot = interp(rotations[current_ts_index], rotations[next_ts_index], i);
      auto const attenuated = interp(n.lrs().rotation, rot, mix_factor);

      n.set_rotation(attenuated);
      break;
    }
    case animation_target::scale:
    {
      auto const& scales = std::get<0>(_checkpoints);
      auto const sca = interp(scales[current_ts_index], scales[next_ts_index], i);
      auto const attenuated = interp(n.lrs().scale, sca, mix_factor);

      n.set_scale(attenuated);
      break;
    }
    }
  }

private:
  size_t _node_index;
  animation_target _target;
  checkpoints _checkpoints;
  std::vector<float> _timestamps;
};

animation::animation(animation_target target, size_t node_index, checkpoints checkpoints, std::vector<float> timestamps)
  : _node_index(node_index), _target(target), _checkpoints(std::move(checkpoints)), _timestamps(std::move(timestamps))
{
}

class joint_animation
{
public:
  struct anim_info
  {
    std::vector<animation> const* animation;
    bool one_shot;
  };

  void append(std::vector<animation> const* anim, bool one_shot = true)
  {
    for (auto const& a : *anim)
      _longest_duration = std::max(_longest_duration, a.duration());
    _anims.push_back(anim_info{ anim, one_shot });
  }

  void start(double offset = 0.0f)
  {
    _time = offset;
    _ramp_up = 0;
    _ramp_up.to(1.0);
    _current = 0;
  }

  void update(std::chrono::duration<double> d, std::vector<transform_node>& nodes)
  {
    _time += d.count();
    _ramp_up.update(d.count());

    bool animation_finished = false;
    if (_longest_duration <= _time)
      animation_finished = true;

    if (animation_finished)
    {
      _time = std::fmodf(_time, _longest_duration);
      if (_anims[_current].one_shot)
        _current = (_current + 1) % _anims.size();
    }

    for (auto& a : *_anims[_current].animation)
    {
      a.transform(_time, float(_ramp_up.value()), nodes);
    }
  }

private:
  std::vector<anim_info> _anims;
  size_t _current;
  double _time = 0;
  float _longest_duration = 0.0f;
  goop::smooth<double> _ramp_up;
};

class transform_tree
{
public:
  transform_tree(std::vector<transform_node> nodes)
    : _nodes(std::move(nodes)), _global_matrices(_nodes.size(), rnu::mat4(1.0f)) {
    recompute_globals();
  }

  void animate(joint_animation& animation, std::chrono::duration<double> delta)
  {
    animation.update(delta, _nodes);
    recompute_globals();
  }

  rnu::mat4 const& global_transform(size_t node) const
  {
    return _global_matrices[node];
  }

private:
  void recompute_globals()
  {
    for (int i = 0; i < _nodes.size(); ++i)
    {
      _global_matrices[i] = _nodes[i].matrix();
    }

    for (int i = 0; i < _nodes.size(); ++i)
    {
      if (_nodes[i].parent != -1)
      {
        _global_matrices[i] = _global_matrices[_nodes[i].parent] * _global_matrices[i];
      }
    }
  }

  std::vector<transform_node> _nodes;
  std::vector<rnu::mat4> _global_matrices;
};

std::pair<ptrdiff_t, size_t> insert_children(std::vector<transform_node>& nodes, std::vector<bool>& done, tinygltf::Model const& model, int node)
{
  auto const& child = model.nodes[node];

  // apply transformation
  if (child.matrix.size() == 16)
  {
    rnu::mat4d mat;
    memcpy(&mat, child.matrix.data(), sizeof(rnu::mat4d));
    nodes[relocations[node]].transformation = rnu::mat4(mat);
  }
  else
  {
    rnu::quatd rot{ 1,0,0,0 };
    rnu::vec3d scale{ 1,1,1 };
    rnu::vec3d trl{ 0,0,0 };
    if (!child.rotation.empty())
    {
      rnu::vec4d rot_tmp;
      memcpy(rot_tmp.data(), child.rotation.data(), sizeof(rnu::vec4d));
      rot = normalize(rnu::quatd(rot_tmp.w, rot_tmp.x, rot_tmp.y, rot_tmp.z));
    }
    if (!child.translation.empty())
      memcpy(trl.data(), child.translation.data(), sizeof(rnu::vec3d));
    if (!child.scale.empty())
      memcpy(scale.data(), child.scale.data(), sizeof(rnu::vec3d));

    transform_node::loc_rot_scale lrs;
    lrs.location = trl;
    lrs.rotation = rot;
    lrs.scale = scale;
    nodes[relocations[node]].transformation = lrs;
  }

  // first, insert all child nodes, then recurse
  auto const base_offset = nodes.size();
  auto count = child.children.size();
  for (auto& c : child.children)
  {
    relocations[c] = nodes.size();
    nodes.emplace_back();
    nodes.back().parent = relocations[node];
  }

  for (int i = 0; i < child.children.size(); ++i)
  {
    auto& n = nodes[base_offset + i];
    auto [offset, count] = insert_children(nodes, done, model, child.children[i]);
    n.children_offset = offset;
    n.num_children = count;
  }

  done[node] = true;
  return { base_offset, count };
}

transform_tree extract_nodes(tinygltf::Model const& model)
{
  std::vector<transform_node> nodes;
  nodes.reserve(model.nodes.size());
  relocations.resize(model.nodes.size());
  std::vector<bool> done(model.nodes.size());

  std::vector<int> root_indices;
  {
    std::vector<bool> is_root(model.nodes.size(), true);
    for (int i = 0; i < is_root.size(); ++i)
    {
      for (auto const& c : model.nodes[i].children)
        is_root[c] = false;
    }

    for (int i = 0; i < is_root.size(); ++i)
      if (is_root[i])
        root_indices.push_back(i);
  }
  std::vector<std::pair<ptrdiff_t, size_t>> roots;

  for (auto i : root_indices)
  {
    relocations[i] = nodes.size();
    nodes.emplace_back();
    nodes.back().parent = -1;
    roots.push_back(insert_children(nodes, done, model, i));
  }
  return transform_tree{ nodes };
}

std::experimental::generator<std::pair<size_t, std::span<char const>>> elements_of(tinygltf::Model const& model, tinygltf::Accessor const& acc) {
  auto const& bufv = model.bufferViews[acc.bufferView];
  auto const& buf = model.buffers[bufv.buffer];
  auto const stride = acc.ByteStride(bufv);
  auto const size = tinygltf::GetComponentSizeInBytes(acc.componentType) * tinygltf::GetNumComponentsInType(acc.type);
  auto const base_ptr = reinterpret_cast<char const*>(buf.data.data()) + bufv.byteOffset + acc.byteOffset;
  for (size_t i = 0; i < acc.count; ++i)
  {
    auto const ptr = base_ptr + i * stride;
    co_yield std::pair<size_t, std::span<char const>>(i, std::span<char const>(ptr, size));
  }
};

std::unordered_map<std::string, std::vector<animation>> extract_animations(tinygltf::Model const& model)
{
  std::unordered_map<std::string, std::vector<animation>> anims;

  for (auto const& anim : model.animations)
  {
    anims[anim.name].reserve(anim.samplers.size());
    auto& dst = anims[anim.name];
    for (int i = 0; i < anim.samplers.size(); ++i)
    {
      tinygltf::AnimationChannel const& channel = anim.channels[i];
      tinygltf::AnimationSampler const& sampler = anim.samplers[channel.sampler];

      auto const node = relocations[channel.target_node];
      auto const type = channel.target_path == "translation" ? animation_target::location : (
        channel.target_path == "rotation" ? animation_target::rotation : (
          channel.target_path == "scale" ? animation_target::scale : animation_target::location
          )
        );
      auto& inacc = model.accessors[sampler.input];

      std::vector<float> timestamps(inacc.count);

      for (auto [item, data] : elements_of(model, inacc))
        memcpy(&timestamps[item], data.data(), data.size());

      auto& outacc = model.accessors[sampler.output];

      animation::checkpoints checkpoints;

      switch (type)
      {
      case animation_target::location:
      case animation_target::scale: checkpoints.emplace<0>(outacc.count); break;
      case animation_target::rotation: checkpoints.emplace<1>(outacc.count); break;
      }

      for (auto [item, data] : elements_of(model, outacc))
      {
        switch (type)
        {
        case animation_target::location:
        case animation_target::scale:
          memcpy(std::get<0>(checkpoints)[item].data(), data.data(), data.size());
          break;
        case animation_target::rotation: {
          rnu::vec4 r;
          memcpy(r.data(), data.data(), data.size());
          std::get<1>(checkpoints)[item] = rnu::normalize(rnu::quat(r.w, r.x, r.y, r.z));
          break;
        }
        }
      }

      dst.emplace_back(type, node, std::move(checkpoints), std::move(timestamps));
    }
  }
  return anims;
}

std::vector<rnu::mat4> extract_joints(tinygltf::Model const& model)
{
  auto const inv_mat_acc = model.accessors[model.skins[0].inverseBindMatrices];

  std::vector<rnu::mat4> joints_base(inv_mat_acc.count);

  for (auto [i, data] : elements_of(model, inv_mat_acc))
    memcpy(&joints_base[i], data.data(), data.size());

  return joints_base;
}

template<std::integral I>
auto const copy_int(char const* src, std::uint32_t& dst)
{
  I i{};
  std::memcpy(&i, src, sizeof(I));
  dst = static_cast<std::uint32_t>(i);
}

std::vector<std::vector<goop::vertex_offset>> extract_geometries(tinygltf::Model const& model, goop::geometry& dst_batch)
{
  std::vector<std::uint32_t> indices;
  std::vector<goop::vertex> vertices;

  std::vector<std::vector<goop::vertex_offset>> material_geometries(model.materials.size());
  for (auto const& mesh : model.meshes)
  {
    for (auto const& prim : mesh.primitives)
    {
      auto const pos_acc = model.accessors[prim.attributes.at("POSITION")];

      vertices.clear();
      vertices.resize(pos_acc.count);

      for (auto [i, data] : elements_of(model, pos_acc))
      {
        vertices[i].color = { 0, 0, 0, 255 };
        memcpy(vertices[i].position.data(), data.data(), data.size());
      }

      if (prim.attributes.contains("NORMAL"))
      {
        for (auto [i, data] : elements_of(model, model.accessors[prim.attributes.at("NORMAL")]))
          memcpy(vertices[i].normal.data(), data.data(), data.size());
      }

      if (prim.attributes.contains("TEXCOORD_0"))
      {
        for (auto [i, data] : elements_of(model, model.accessors[prim.attributes.at("TEXCOORD_0")]))
          memcpy(vertices[i].uv[0].data(), data.data(), data.size());
      }

      if (prim.attributes.contains("JOINTS_0"))
      {
        for (auto [i, data] : elements_of(model, model.accessors[prim.attributes.at("JOINTS_0")]))
          memcpy(vertices[i].joints.data(), data.data(), data.size());
        for (auto [i, data] : elements_of(model, model.accessors[prim.attributes.at("WEIGHTS_0")]))
          memcpy(vertices[i].weights.data(), data.data(), data.size());
      }

      auto const idx_acc = model.accessors[prim.indices];
      indices.clear();
      indices.resize(idx_acc.count);

      for (auto [i, data] : elements_of(model, idx_acc))
      {
        switch (idx_acc.componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
          copy_int<std::uint16_t>(data.data(), indices[i]);
          break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
          copy_int<std::uint32_t>(data.data(), indices[i]);
          break;
        }
      }
      material_geometries[prim.material].push_back(dst_batch->append_vertices(vertices, indices));
    }
  }
  return material_geometries;
}

std::vector<material> extract_materials(tinygltf::Model const& model, goop::texture_provider& provider)
{
  std::vector<material> materials(model.materials.size());

  for (size_t i = 0; i < model.materials.size(); ++i)
  {
    auto diffuse_texture_index = model.materials[i].pbrMetallicRoughness.baseColorTexture.index;

    auto const ext_specular_glossiness = model.materials[i].extensions.find("KHR_materials_pbrSpecularGlossiness");
    if (diffuse_texture_index == -1)
    {
      if (ext_specular_glossiness != model.materials[i].extensions.end())
      {
        if(ext_specular_glossiness->second.Has("diffuseTexture"))
          diffuse_texture_index = ext_specular_glossiness->second.Get("diffuseTexture").Get("index").GetNumberAsInt();
        // Todo: Whatever
        else if(ext_specular_glossiness->second.Has("specularGlossinessTexture"))
          diffuse_texture_index = ext_specular_glossiness->second.Get("specularGlossinessTexture").Get("index").GetNumberAsInt();
      }
    }

    rnu::vec4 diff_factor{0,0,0,0};
    if (model.materials[i].pbrMetallicRoughness.baseColorFactor.size() == 4)
    {
      diff_factor[0] = model.materials[i].pbrMetallicRoughness.baseColorFactor[0];
      diff_factor[1] = model.materials[i].pbrMetallicRoughness.baseColorFactor[1];
      diff_factor[2] = model.materials[i].pbrMetallicRoughness.baseColorFactor[2];
      diff_factor[3] = model.materials[i].pbrMetallicRoughness.baseColorFactor[3];
    }

    if (ext_specular_glossiness != model.materials[i].extensions.end())
    {
      auto const diff_fac = ext_specular_glossiness->second.Get("diffuseFactor");
      diff_factor[0] = diff_fac.Get(0).GetNumberAsDouble();
      diff_factor[1] = diff_fac.Get(1).GetNumberAsDouble();
      diff_factor[2] = diff_fac.Get(2).GetNumberAsDouble();
      diff_factor[3] = diff_fac.Get(3).GetNumberAsDouble();
    }
    materials[i].data.color = diff_factor;

    if (diffuse_texture_index != -1)
    {
      auto& img = model.images[model.textures[diffuse_texture_index].source];

      materials[i].diffuse_map->allocate(goop::texture_type::t2d, goop::data_type::rgba8unorm, img.width, img.height, goop::compute_texture_mipmap_count);
      materials[i].diffuse_map->set_data(0, 0, 0, img.width, img.height, 4, img.image);
      materials[i].diffuse_map->generate_mipmaps();
      materials[i].data.has_texture = true;
    }
    else
    {
      materials[i].data.has_texture = false;
    }
  }
  return materials;
}

class skin
{
public:
  skin(size_t root, std::vector<std::uint32_t> joint_nodes, std::vector<rnu::mat4> joint_matrices)
    : _root_node(root), _joint_nodes(std::move(joint_nodes)), _joint_matrices(std::move(joint_matrices)) {
  }

  std::uint32_t joint_node(size_t index) const { return _joint_nodes[index]; }
  rnu::mat4 const& joint_matrix(size_t index) const { return _joint_matrices[index]; }
  size_t size() const { return _joint_nodes.size(); }

  std::vector<rnu::mat4>& apply_global_transforms(transform_tree const& tree)
  {
    _global_joint_matrices.resize(size(), rnu::mat4(1.0f));

    for (int i = 0; i < size(); ++i)
    {
      auto const& g = tree.global_transform(joint_node(i));
      _global_joint_matrices[i] = g * joint_matrix(i);
    }
    return _global_joint_matrices;
  }

private:
  size_t _root_node;
  std::vector<std::uint32_t> _joint_nodes;
  std::vector<rnu::mat4> _joint_matrices;
  std::vector<rnu::mat4> _global_joint_matrices;
};

std::vector<skin> extract_skins(tinygltf::Model const& model)
{
  std::vector<skin> result;
  result.reserve(model.skins.size());
  for (auto const& s : model.skins)
  {
    auto const inv_mat_acc = model.accessors[s.inverseBindMatrices];

    std::vector<rnu::mat4> joints_base(inv_mat_acc.count);

    for (auto [i, data] : elements_of(model, inv_mat_acc))
      memcpy(&joints_base[i], data.data(), data.size());

    auto const root = s.skeleton;

    std::vector<std::uint32_t> joint_nodes(s.joints.size());
    for (size_t i = 0; i < s.joints.size(); ++i)
      joint_nodes[i] = relocations[s.joints[i]];

    result.emplace_back(root, std::move(joint_nodes), std::move(joints_base));
  }
  return result;
}

int main()
{
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
  goop::default_app app("Model");
  glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    std::cout << message << '\n';
    }, nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);

  goop::geometry main_geometry;
  goop::mapped_buffer<rnu::mat4> joints_buf;
  
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  std::filesystem::path const duck = "../../../../../res/ptcl/scene.gltf";
  bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, duck.string());

  std::cerr << "GLTF [E]: " << err << '\n';
  std::cerr << "GLTF [W]: " << warn << '\n';

  auto nodes = extract_nodes(model);
  auto materials = extract_materials(model, app.default_texture_provider());
  auto skins = extract_skins(model);

  const auto anims = extract_animations(model);
  const auto material_geometries = extract_geometries(model, main_geometry);

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
  std::array<goop::mapped_buffer<matrices>, 2> uniform_buffer = { goop::mapped_buffer<matrices>{1}, goop::mapped_buffer<matrices>{1} };

  int img = 0;

  joint_animation run;
 run.append(&anims.at("Running"), false);

  joint_animation idle;
  idle.append(&anims.at("Idle"), true);
  // idle.append(&anims.begin()->second, true);
  idle.start();

  joint_animation* running_anim = &idle;

  struct _key
  {
    GLFWwindow* w;
    int key;
    bool _is = false;

    enum class state
    {
      up,
      press,
      down,
      release
    };

    _key(GLFWwindow* w, int key) : w(w), key(key) {}

    state update() {
      bool const is = glfwGetKey(w, key) == GLFW_PRESS;
      if (_is != is)
      {
        _is = is;
        return _is ? state::press : state::release;
      }
      else
      {
        return _is ? state::down : state::up;
      }
    }
  };

  _key anim_key(app.window().get(), GLFW_KEY_O);
  _key restart_anim(app.window().get(), GLFW_KEY_SPACE);

  while (app.begin_frame())
  {
    auto [window_width, window_height] = app.default_draw_state()->current_surface_size();

    auto& state = app.default_draw_state();
    state->set_depth_test(true);
    pipeline->use(vert);
    pipeline->use(frag);
    pipeline->bind(state);

    auto const& camera = app.default_camera();
    auto const view_mat = camera.matrix();
    auto const proj_mat = rnu::cameraf::projection(rnu::radians(60.f), window_width / float(window_height), 0.01f, 1000.f);

    uniform_buffer[img]->write(matrices{view_mat, proj_mat});
    uniform_buffer[img]->bind(state, 0);

    switch (restart_anim.update())
    {
    case _key::state::press:
      run.start();
      running_anim = &run;
      break;
    case _key::state::release:
      idle.start(0.0);
      running_anim = &idle;
      break;
    }

    if (!anims.empty() && running_anim)
      nodes.animate(*running_anim, app.current_delta_time());
    
    joints_buf->write(skins[0].apply_global_transforms(nodes));
    joints_buf->bind(state, 1);
    sampler->bind(state, 0);
    for (size_t i = 0; i < materials.size(); ++i)
    {
      materials[i].bind(state);
      main_geometry->draw(app.default_draw_state(), material_geometries[i]);
    }

    img = app.end_frame();
  }
}