#include "panel_2d.hpp"
#include <iostream>

namespace goop::gui
{
#define PANEL_INFO_BUFFER_STR R"(
  layout(binding = 0) buffer Info
  {
    vec2 position;
    vec2 size;
    vec2 uv_bottom_left;
    vec2 uv_top_right;
    uint color;
    uint has_texture;
  } info;
)"

  constexpr auto vv = R"(#version 450 core

out gl_PerVertex
{
	vec4 gl_Position;
};

)" PANEL_INFO_BUFFER_STR R"(

layout(location = 0) out vec2 uv;

void main()
{
  float x = gl_VertexID & 1;
  float y = (gl_VertexID & 2) >> 1;

  uv = mix(info.uv_bottom_left, info.uv_top_right, vec2(x, y));
  gl_Position = vec4(vec2(x, y) * 2 - 1, 0, 1);
}

)";

  constexpr auto ff = R"(#version 450 core
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
layout(binding = 0) uniform sampler2D tex;
)" PANEL_INFO_BUFFER_STR R"(

void main()
{
  vec4 c = unpackUnorm4x8(info.color);
  vec4 t = vec4(1,1,1,1);
  if(info.has_texture != 0)
    t = texture(tex, uv);
  color = c * t;
  //color.rgb *= color.a;
}
)";

  void panel_2d::set_position(rnu::vec2 position) {
    _info.set(&panel_info::position, position);
  }

  void panel_2d::set_size(rnu::vec2 size) {
    _info.set(&panel_info::size, size);
  }

  void panel_2d::set_color(rnu::vec4 color)
  {
    _info.set(&panel_info::color, norm_color(color));
  }

  void panel_2d::set_uv(rnu::vec2 bottom_left, rnu::vec2 top_right)
  {
    _info.set(&panel_info::uv_bottom_left, bottom_left);
    _info.set(&panel_info::uv_top_right, top_right);
  }

  void panel_2d::set_texture(std::optional<texture> texture)
  {
    _texture = std::move(texture);
    _info.set(&panel_info::has_texture, std::uint32_t(bool(_texture)));
  }

  void panel_2d::draw(draw_state_base& state)
  {
    if (_info.info().color == rnu::vec4(0, 0, 0, 0))
      return;

    thread_local geometry_format geometry; // no attributes
    thread_local shader_pipeline pipeline = [] {
      std::string shader_log;
      goop::shader vs(goop::shader_type::vertex, vv, &shader_log);
      std::cout << "VSLOG 2: " << shader_log << '\n';
      goop::shader fs(goop::shader_type::fragment, ff, &shader_log);
      std::cout << "FSLOG 2: " << shader_log << '\n';

      shader_pipeline p;
      p->use(vs);
      p->use(fs);
      return p;
    }();
    thread_local sampler s = [] {
      sampler s;
      s->set_clamp(goop::wrap_mode::clamp_to_edge);
      s->set_min_filter(goop::sampler_filter::linear, goop::sampler_filter::linear);
      s->set_mag_filter(goop::sampler_filter::linear);
      s->set_max_anisotropy(16);
      return s;
    }();

    auto const [w, h] = state.current_surface_size();
    auto pos = _info.info().position;
    //pos.y = h - pos.y - _info.info().size.y;

    state.set_depth_test(false);
    state.set_viewport({ pos, _info.info().size });
    state.set_scissor(rnu::rect2f{ pos, _info.info().size });
    state.set_culling_mode(culling_mode::none);
    state.set_blending(blending_mode{
      .src_color = {false, blending::src_alpha},
      .dst_color = {true, blending::src_alpha},
      .src_alpha = {false, blending::one},
      .dst_alpha = {true, blending::src_alpha}, // 0 + dst * src
      .equation_color = blending_equation::src_plus_dst,
      .equation_alpha = blending_equation::src_plus_dst
      });
    if (_info.dirty())
    {
      _info_buffer->write(_info.info());
      _info.clear();
    }

    pipeline->bind(state);
    _info_buffer->bind(state, 0);
    if (_texture)
    {
      _texture.value()->bind(state, 0);
      s->bind(state, 0);
    }
    geometry->draw_array(state, goop::primitive_type::triangle_strip, draw_info_array{ 4, 1, 0, 0 });
    state.set_scissor(std::nullopt);
    state.set_blending(std::nullopt);
  }
}