#include "default_app.hpp"
#include <glad/glad.h>
#include <rnu/math/math.hpp>
#include "graphics.hpp"
#include <rnu/camera.hpp>
#include <iostream>
#include <array>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include "generic/texture_provider.hpp"
#include "dynamic_octree.hpp"
#include "algorithm/perlin.hpp"
#include "algorithm/looper.hpp"
#include <map>
#include <future>
#include <thread>
#include <queue>
#include "algorithm/cull.hpp"

#include <shadow.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct window_deleter
{
  void operator()(GLFWwindow* window) const {
    glfwDestroyWindow(window);
  }
};
using window_ptr = std::unique_ptr<GLFWwindow, window_deleter>;

static struct glfw_static_context
{
  glfw_static_context() { glfwInit(); }
  ~glfw_static_context() { glfwTerminate(); }
} const glfw_context;

constexpr auto window_title = "goop";
constexpr auto start_width = 1600;
constexpr auto start_height = 900;

struct matrices
{
  rnu::mat4 view;
  rnu::mat4 proj;
};

constexpr auto bg_vs = R"(
#version 460 core

layout(location = 0) in vec3 position;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(location = 0) out vec2 uv;

void main()
{
  gl_Position = vec4(position, 1);
  uv = position.xy;
}
)";

constexpr auto bg_fs = R"(
#version 460 core

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

layout(std430, binding = 0) buffer Matrices
{
  mat4 view;
  mat4 proj;
};

void main()
{
  mat4 mat_mul2 = proj * mat4(mat3(view));
  vec4 ray_direction_hom = (inverse(mat_mul2) * vec4(uv, 1, 1));
  vec3 ray_direction = normalize(ray_direction_hom.xyz);

  vec3 sky = mix(vec3(0,0,0), vec3(0.5f, 0.7f, 1.0f), smoothstep(-1.0, 0.0, ray_direction.y));
  color = vec4(sky, 1);
}
)";

constexpr auto vertex_shader_source = R"(
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv[3];
layout(location = 5) in vec4 color;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(std430, binding = 0) buffer Matrices
{
  mat4 view;
  mat4 proj;
};

layout(location = 0) out vec3 pass_position;
layout(location = 1) out vec3 pass_normal;
layout(location = 2) out vec2 pass_uv[3];
layout(location = 5) out vec4 pass_color;
layout(location = 6) out vec3 pass_view_position;

void main()
{
  vec4 hom_position = vec4(position, 1);
  pass_view_position = (view * hom_position).xyz;
  gl_Position = proj * view * hom_position;

  pass_position = position;
  pass_normal = normal;
  pass_uv[0] = uv[0];
  pass_uv[1] = uv[1];
  pass_uv[2] = uv[2];
  pass_color = color;
}
)";

constexpr auto fragment_shader_source = R"(
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv[3];
layout(location = 5) in vec4 color;


layout(std430, binding = 0) buffer Matrices
{
  mat4 view;
  mat4 proj;
};

layout(std430, binding = 1) buffer ShadowMatrices
{
  mat4 shadow_view;
  mat4 shadow_proj;
};

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D image_texture;
layout(binding = 1) uniform sampler2DShadow shadow_map;

void main()
{
  vec4 hpos = vec4(position, 1);
  vec4 npos = shadow_proj * shadow_view * hpos;
  npos.xyz /= npos.w;

  float shadow = texture(shadow_map, vec3(npos.xy / 2 + 0.5, npos.z));

  vec3 ldir = normalize(vec3(1,0.7,0.5));
  vec3 col = texture(image_texture, uv[0]).rgb;
  col = col * max(0, dot(ldir, normalize(normal))) + col * vec3(0.5f, 0.7f, 1.0f);  

  vec3 campos = inverse(view)[3].rgb;
  vec3 ctp = position - campos;
  float len = length(ctp);
  vec3 view_dir = normalize(ctp);
  vec3 half_vec = normalize(-view_dir + ldir);

  col += 0.5 * pow(max(0, dot(half_vec, normalize(normal))), 80);

  vec3 sky = mix(vec3(0,0,0), vec3(0.5f, 0.7f, 1.0f), smoothstep(-1.0, 0.0, view_dir.y));

  out_color = shadow * vec4(mix(col, sky, smoothstep(110, 128,len)), 1);
}
)";

std::uint16_t block_at(int x, int y, int z)
{
  float const perlin = goop::perlin_noise(x / 80.f, 0, z / 80.f, 6, 0.5f);
  float const perlinx = goop::perlin_noise(x / 50.f, y / 80.f, z / 50.f, 4, 0.5f);

  float h = y - (-22 + 120 * (perlin));

  if (h > 0)
    return 0;

  if (perlinx > 0.53 && perlinx < 0.7)
    return 0;

  if (h <= 0 && h > -8)
  {
    if (h > -2)
      return 2;
    return 1;
  }

  if (perlinx > 0.49 && perlinx < 0.75)
    return 3;

  float const perlin2 = goop::perlin_noise(x / 70.f, y / 70.f, z / 70.f);
  return int(3 * perlin2) % 3 + 1;
}

class vertex_provider
{
public:
  struct vertices
  {
    std::unordered_map<std::uint16_t, std::vector<goop::vertex>> stage_vertices;
    std::unordered_map<std::uint16_t, std::vector<std::uint32_t>> stage_indices;
  };

  vertices alloc()
  {
    std::unique_lock lock(_mutex);
    if (_unused.empty())
      return vertices{};
    else
    {
      auto val = std::move(_unused.back());
      _unused.pop_back();
      return val;
    }
  }

  void free(vertices&& vert)
  {
    std::unique_lock lock(_mutex);
    for (auto& [k, v] : vert.stage_vertices)
      v.clear();
    for (auto& [k, i] : vert.stage_indices)
      i.clear();
    _unused.push_back(std::move(vert));
  }

private:
  std::mutex _mutex;
  std::vector<vertices> _unused;
};

class chunk
{
public:
  static constexpr auto gen_border_size = 2 * 2 * 2 * 2 * 2;

  chunk()
    : _chunk(5) {

  }

  static bool visible(rnu::mat4 view_projection, float offsetx, float offsety, float offsetz)
  {
    rnu::vec3 min{ offsetx , offsety, offsetz };
    rnu::vec3 max{ offsetx + gen_border_size, offsety + gen_border_size, offsetz + gen_border_size };

    return goop::cull_aabb(view_projection, min, max) != goop::cull_result::fully_outside;
  }

  std::uint16_t block_at(int x, int y, int z) const {
    return _chunk.at(x, y, z);
  }

  void set_block(int x, int y, int z, std::uint16_t id)
  {
    _chunk.emplace(x, y, z, id);
  }
  
  std::size_t border_size() const
  {
    return _chunk.border_size();
  }

  void regenerate(vertex_provider& vertex_provider, float offsetx, float offsety, float offsetz)
  {
    auto const block_is_opaque = [&](rnu::vec3i pos)
    {
      int const x = pos.x;
      int const y = pos.y;
      int const z = pos.z;

      return ::block_at(x, y, z) != 0;
    };

    constexpr std::array const zpos{
        goop::vertex{.position = {-0.5, -0.5,  0.5}, .normal = { 0, 0, 1 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
        goop::vertex{.position = {-0.5,  0.5,  0.5}, .normal = { 0, 0, 1 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
        goop::vertex{.position = { 0.5, -0.5,  0.5}, .normal = { 0, 0, 1 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
        goop::vertex{.position = { 0.5,  0.5,  0.5}, .normal = { 0, 0, 1 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} },
    };
    constexpr std::array const zneg{
        goop::vertex{.position = {-0.5, -0.5, -0.5}, .normal = { 0, 0, -1 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
        goop::vertex{.position = { 0.5, -0.5, -0.5}, .normal = { 0, 0, -1 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
        goop::vertex{.position = {-0.5,  0.5, -0.5}, .normal = { 0, 0, -1 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
        goop::vertex{.position = { 0.5,  0.5, -0.5}, .normal = { 0, 0, -1 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} }
    };
    constexpr std::array const ypos{
        goop::vertex{.position = {-0.5, 0.5, -0.5}, .normal = { 0, 1, 0  }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
        goop::vertex{.position = { 0.5, 0.5, -0.5}, .normal = { 0, 1, 0  }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
        goop::vertex{.position = {-0.5, 0.5,  0.5}, .normal = { 0, 1, 0  }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
        goop::vertex{.position = { 0.5, 0.5,  0.5}, .normal = { 0, 1, 0  }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} },
    };
    constexpr std::array const yneg{
        goop::vertex{.position = {-0.5, -0.5, -0.5}, .normal = { 0, -1, 0 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
        goop::vertex{.position = {-0.5, -0.5,  0.5}, .normal = { 0, -1, 0 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
        goop::vertex{.position = { 0.5, -0.5, -0.5}, .normal = { 0, -1, 0 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
        goop::vertex{.position = { 0.5, -0.5,  0.5}, .normal = { 0, -1, 0 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} }
    };
    constexpr std::array const xpos{
        goop::vertex{.position = {0.5, -0.5, -0.5}, .normal = { 1, 0, 0 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
        goop::vertex{.position = {0.5, -0.5,  0.5}, .normal = { 1, 0, 0 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
        goop::vertex{.position = {0.5,  0.5, -0.5}, .normal = { 1, 0, 0 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
        goop::vertex{.position = {0.5,  0.5,  0.5}, .normal = { 1, 0, 0 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} },
    };
    constexpr std::array const xneg{
        goop::vertex{.position = {-0.5, -0.5, -0.5}, .normal = { -1, 0, 0 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
        goop::vertex{.position = {-0.5,  0.5, -0.5}, .normal = { -1, 0, 0 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
        goop::vertex{.position = {-0.5, -0.5,  0.5}, .normal = { -1, 0, 0 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
        goop::vertex{.position = {-0.5,  0.5,  0.5}, .normal = { -1, 0, 0 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} }
    };
    constexpr std::array const indices{
      0u, 2u, 1u,
      1u, 2u, 3u
    };
    constexpr std::array const faces{
      zpos,
      zneg,
      ypos,
      yneg,
      xpos,
      xneg,
    };

    auto const start_time = std::chrono::steady_clock::now();

    auto vertex_alloc = vertex_provider.alloc();
    auto& [stage_vertices, stage_indices] = vertex_alloc;

    for (auto& [key, val] : _block_geometries)
      val.clear();
    std::array<goop::vertex, 6 * 6> vertices;
    std::array<std::uint32_t, 6 * 6> ids;
    for (int x = 0; x < _chunk.border_size(); ++x)
    {
      for (int z = 0; z < _chunk.border_size(); ++z)
      {
        for (int y = 0; y < _chunk.border_size(); ++y)
        {
          auto const info = _chunk.info(x, y, z);
          auto const val = info.value;
          if (val == 0) {
            y += info.size - 1;
            continue;
          }

          const std::array neighbors{
            rnu::vec3i{ x, y, z + 1 },
            rnu::vec3i{ x, y, z - 1 },
            rnu::vec3i{ x, y + 1, z },
            rnu::vec3i{ x, y - 1, z },
            rnu::vec3i{ x + 1, y, z },
            rnu::vec3i{ x - 1, y, z }, };

          auto* v_insert = vertices.data();
          auto* i_insert = ids.data();

          for (int i = 0; i < faces.size(); ++i)
          {
            auto const base_pos = rnu::vec3{ float(x) + offsetx, float(y) + offsety, float(z) + offsetz };
            if (block_is_opaque(neighbors[i] + rnu::vec3(offsetx, offsety, offsetz)))
              continue;

            auto const dist = std::distance(vertices.data(), v_insert);

            for (auto idx : indices)
            {
              *i_insert = idx + dist;
              i_insert++;
            }

            auto face = faces[i];
            for (auto& v : face)
            {
              v.position = (v.position + rnu::vec3{ float(x) + offsetx, float(y) + offsety, float(z) + offsetz });
              *v_insert = v;
              v_insert++;
            }
          }

          auto vertex_range = std::span(vertices).subspan(0, std::distance(vertices.data(), v_insert));
          auto index_range = std::span(ids).subspan(0, std::distance(ids.data(), i_insert));

          if (!vertex_range.empty() && !index_range.empty())
          {
            auto& iv = stage_indices[val];
            auto& vv = stage_vertices[val];
            for (auto& i : index_range)
              iv.push_back(i + vv.size());
            for (auto& v : vertex_range)
              vv.push_back(v);
          }
        }
      }
    }

    _geometry->clear();
    for (auto& [k, v] : stage_vertices)
    {
      if(!v.empty())
        _block_geometries[k].push_back(_geometry->append_vertices(v, stage_indices[k]));
    }
    vertex_provider.free(std::move(vertex_alloc));

    auto const end_time = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count() << " seconds for generation\n";
  }

  goop::geometry& geometry() { return _geometry; }

  std::unordered_map<std::uint16_t, std::vector<goop::vertex_offset>> const& block_geometries() const{ return _block_geometries; }

private:
  goop::geometry _geometry;
  std::unordered_map<std::uint16_t, std::vector<goop::vertex_offset>> _block_geometries;
  goop::dynamic_octree<std::uint16_t> _chunk;
};


class world
{
public:
  std::shared_ptr<chunk> at(int x, int y, int z)
  {
    std::tuple index{ x, y, z };

    auto val_iter = _chunks.find(index);
    if (val_iter == _chunks.end())
    {
      auto const iter = _chunk_loaders.find(index);

      if (iter == _chunk_loaders.end())
      {
        _chunk_loaders.emplace(index, _looper.async([&, x, y, z] {
          auto v = std::make_shared<chunk>();
          generate(*v, x, y, z);
          return v;
          }));
        return nullptr;
      }

      auto& fut = iter->second;
      if (goop::is_ready(fut))
      {
        auto const begin = std::chrono::steady_clock::now();
        auto [iter, success] = _chunks.emplace(index, std::move(fut.get()));
        _chunk_loaders.erase(index);

        auto const end = std::chrono::steady_clock::now();
        return iter->second;
      }
      else
      {
        return nullptr;
      }
    }
    return val_iter->second;
  }

private:
  void generate(chunk& c, int chx, int chy, int chz)
  {
    auto const base_x = chx * static_cast<int>(c.border_size());
    auto const base_y = chy * static_cast<int>(c.border_size());
    auto const base_z = chz * static_cast<int>(c.border_size());

    for (int x = 0; x < c.border_size(); ++x)
      for (int y = 0; y < c.border_size(); ++y)
        for (int z = 0; z < c.border_size(); ++z)
        {
          auto const xr = base_x + x;
          auto const yr = base_y + y;
          auto const zr = base_z + z;

          auto const block = block_at(xr, yr, zr);
          if(block != 0)
            c.set_block(x, y, z, block);
        }
    c.regenerate(_vertex_provider, base_x, base_y, base_z);
  }

  vertex_provider _vertex_provider;
  goop::looper _looper{ 4 };
  std::map<std::tuple<int, int, int>, std::shared_ptr<chunk>> _chunks;
  std::map<std::tuple<int, int, int>, std::future<std::shared_ptr<chunk>>> _chunk_loaders;
};

int main()
{
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

  goop::default_app app(window_title);

  glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    std::cout << message << '\n';
    }, nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);

  std::array<goop::mapped_buffer<matrices>, 2> uniform_buffer = { goop::mapped_buffer<matrices>{1}, goop::mapped_buffer<matrices>{1} };

  std::string info_log;

  goop::shader vertex_shader(goop::shader_type::vertex, vertex_shader_source, &info_log);
  std::cout << "Vertex shader output:\n" << info_log << '\n';
  goop::shader fragment_shader(goop::shader_type::fragment, fragment_shader_source, &info_log);
  std::cout << "Fragment shader output:\n" << info_log << '\n';
  goop::shader_pipeline pipeline;

  goop::shader background_vs(goop::shader_type::vertex, bg_vs, &info_log);
  std::cout << "Vertex shader output:\n" << info_log << '\n';
  goop::shader background_fs(goop::shader_type::fragment, bg_fs, &info_log);
  std::cout << "Fragment shader output:\n" << info_log << '\n';
  goop::shader_pipeline background_pipeline;
  goop::geometry background_geometry;
  
  constexpr std::array background{
    goop::vertex{.position = {-1,-1, 0} },
    goop::vertex{.position = {-1,1, 0} },
    goop::vertex{.position = {1,-1, 0} },
    goop::vertex{.position = {1,1, 0} },
  };
  constexpr std::array indices{
    0u, 2u, 1u,
    1u, 2u, 3u
  };
  auto background_offset = background_geometry->append_vertices(background, indices);

  world w;

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

  goop::sampler sampler;
  sampler->set_clamp(goop::wrap_mode::clamp_to_edge);
  sampler->set_min_filter(goop::sampler_filter::linear, goop::sampler_filter::linear);
  sampler->set_mag_filter(goop::sampler_filter::linear);
  sampler->set_max_anisotropy(16);

  goop::sampler shadow_sampler;
  shadow_sampler->set_clamp(goop::wrap_mode::clamp_to_edge);
  shadow_sampler->set_min_filter(goop::sampler_filter::linear);
  shadow_sampler->set_mag_filter(goop::sampler_filter::linear);
  shadow_sampler->set_compare_fun(goop::compare::less);

  std::unordered_map<std::uint16_t, goop::texture> block_textures{
    std::pair<std::uint16_t, goop::texture>{1, tex("../../../../../res/dirt.png")},
    std::pair<std::uint16_t, goop::texture>{2, tex("../../../../../res/grass.jpg")},
    std::pair<std::uint16_t, goop::texture>{3, tex("../../../../../res/granite.jpg")},
  };

  int img = 0;

  //goop::render_target target;

  goop::shadow shadow(2048, 2048, app.default_texture_provider());

  // shadow camera matrix...
  rnu::mat4 sortho = rnu::cameraf::orthographic(-80, 80, -80, 80, -200, 200);
  auto scam = inverse(rnu::rotation(rnu::quat(rnu::vec3(1, 0, 0), rnu::radians(-90.f))));
  std::array<goop::mapped_buffer<matrices>, 2> camub = { goop::mapped_buffer<matrices>{1}, goop::mapped_buffer<matrices>{1} };

  while (app.begin_frame())
  {
    auto [window_width, window_height] = app.default_draw_state()->current_surface_size();

    auto delta = app.current_delta_time();
    auto const& camera = app.default_camera();
    auto const view_mat = camera.matrix();
    auto const proj_mat = rnu::cameraf::projection(rnu::radians(60.f), window_width / float(window_height), 0.01f, 1000.f);

    uniform_buffer[img]->map();
    uniform_buffer[img]->at(0).view = view_mat;
    uniform_buffer[img]->at(0).proj = proj_mat;
    uniform_buffer[img]->flush();
    uniform_buffer[img]->unmap();

    auto& state = app.default_draw_state();
    state->set_depth_test(false);
    background_pipeline->use(background_vs);
    background_pipeline->use(background_fs);
    background_pipeline->bind(state);
    uniform_buffer[img]->bind(state, 0);
    background_geometry->draw(state, background_offset);
    app.default_render_target()->deactivate(state);

    camub[img]->map();
    camub[img]->at(0).view = scam;
    camub[img]->at(0).proj = sortho;
    camub[img]->flush();
    camub[img]->unmap();

    goop::texture shadow_texture;
    for (int i = 0; i < 2; ++i)
    {
      auto view = i == 0 ? scam : view_mat;
      auto proj = i == 0 ? sortho : proj_mat;

      if (i == 0)
      {
        state->set_viewport(0, 0, 2048, 2048);
        shadow.begin(state, proj * view);
        glColorMaski(0,
          GL_FALSE,
          GL_FALSE,
          GL_FALSE,
          GL_FALSE);
      }
      else
      {
        state->set_viewport(0, 0, window_width, window_height);
        app.default_render_target()->activate(state);
      }

      state->set_depth_test(true);
      pipeline->use(vertex_shader);
      pipeline->use(fragment_shader);
      pipeline->bind(state);
      (i==0 ? camub : uniform_buffer)[img]->bind(state, 0);
      if (i == 1)
        camub[img]->bind(state, 1);
      sampler->bind(state, 0);
      if (i == 1)
      {
        shadow_sampler->bind(state, 1);
        shadow_texture->bind(state, 1);
      }

      rnu::vec3 cam_pos = camera.position() - 0.5f * chunk::gen_border_size;
      cam_pos /= chunk::gen_border_size;
      rnu::vec3i cam_posi(cam_pos);

      constexpr auto radius = 4;
      for (int cx = cam_posi.x - radius; cx <= cam_posi.x + radius; ++cx)
      {
        for (int cy = -radius; cy <= radius; ++cy)
        {
          for (int cz = cam_posi.z - radius; cz <= cam_posi.z + radius; ++cz)
          {
            if (chunk::visible(proj * view, cx * chunk::gen_border_size, cy * chunk::gen_border_size, cz * chunk::gen_border_size))
            {
              auto ch = w.at(cx, cy, cz);
              if (ch)
                for (auto& [block_id, geometries] : ch->block_geometries())
                {
                  block_textures[block_id]->bind(state, 0);
                  ch->geometry()->draw(state, geometries);
                }
            }
          }
        }
      }

      if (i == 0)
      {
        shadow_texture = shadow.end(state);
        glColorMaski(0,
          GL_TRUE,
          GL_TRUE,
          GL_TRUE,
          GL_TRUE);
      }
      else
        app.default_render_target()->deactivate(state);
    }

    img = app.end_frame();
  }

  return 0;
}
