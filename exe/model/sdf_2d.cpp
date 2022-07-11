#include "sdf_2d.hpp"
#include <iostream>

namespace goop::gui
{
#define SDF_INFO_BUFFER_STR R"(
  layout(binding = 0) buffer Info
  {
    vec2 resolution;
    float scale;
    float border_width;

    uint color;
    uint border_color;
    float border_offset;
    float outer_smoothness;

    float inner_smoothness;
    float sdf_width;
  } info;
)"

constexpr auto vv = R"(#version 450 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 size;
layout(location = 2) in vec2 offset;
layout(location = 3) in vec2 uv_size;
layout(location = 4) in vec2 uv_offset;

)"
SDF_INFO_BUFFER_STR
R"(

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(location = 0) out vec2 uv;

void main()
{
  vec2 ncoord = 2 * ((info.scale * (position * size + offset)) / info.resolution) - 1;
  uv = position * uv_size + uv_offset;
  gl_Position = vec4(ncoord, 0.5, 1);
}
)";

  constexpr auto ff = R"(#version 450 core

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D atlas;

)"
SDF_INFO_BUFFER_STR
R"(

void main()
{
  float a = texture(atlas, uv).r;
  float b = 0;  

  float sdf_width = info.sdf_width;

  float smoothness_inner = info.inner_smoothness / info.scale / sdf_width;
  float smoothness_outer = info.outer_smoothness / info.scale / sdf_width;
  float half_smoothness_inner = smoothness_inner / 2.0;
  float half_smoothness_outer = smoothness_outer / 2.0;

  float border = info.border_width / info.scale / sdf_width;
  float half_border = border / 2.0;
  float border_offset = info.border_offset / info.scale / sdf_width; 

  b = smoothstep(0.5 + border_offset + half_border - half_smoothness_inner, 0.5 + border_offset + half_border + half_smoothness_inner, a);
  a = smoothstep(0.5 + border_offset - half_border - half_smoothness_outer, 0.5 + border_offset - half_border + half_smoothness_outer, a);

  vec4 inner_color = unpackUnorm4x8(info.color);
  vec4 border_color = unpackUnorm4x8(info.border_color);
  
  vec4 mixed_color = mix(border_color, inner_color, border == 0.0 ? 1.0 : b);

  color = vec4(mixed_color.rgb, mixed_color.a * a);
}
)";

  void sdf_2d::draw(draw_state_base& state, int x, int y, int w, int h)
  {
    thread_local buffer vtb{ rnu::vec2{0, 0}, rnu::vec2{1, 0}, rnu::vec2{0, 1}, rnu::vec2{1, 1} };
    thread_local buffer idb{ 0u, 1u, 2u, 2u, 1u, 3u };
    thread_local geometry_format geo = [] {
      geometry_format geo;
      geo->set_attribute(0, goop::attribute_for<rnu::vec2, false>(0, 0));
      geo->set_binding(0, sizeof(rnu::vec2), attribute_repetition::per_vertex);
      geo->set_attribute(1, goop::attribute_for<false>(1, &sdf_instance::scale));
      geo->set_attribute(2, goop::attribute_for<false>(1, &sdf_instance::offset));
      geo->set_attribute(3, goop::attribute_for<false>(1, &sdf_instance::uv_scale));
      geo->set_attribute(4, goop::attribute_for<false>(1, &sdf_instance::uv_offset));
      geo->set_binding(1, sizeof(sdf_instance), attribute_repetition::per_instance);
      return geo;
    }();
    thread_local shader_pipeline pipeline = [&] {
      std::string shader_log;
      shader_pipeline pipeline2;
      goop::shader vert2(goop::shader_type::vertex, vv, &shader_log);
      std::cout << "VSLOG 2: " << shader_log << '\n';
      goop::shader frag2(goop::shader_type::fragment, ff, &shader_log);
      std::cout << "FSLOG 2: " << shader_log << '\n';
      pipeline2->use(vert2);
      pipeline2->use(frag2);
      return pipeline2;
    }();

    thread_local sampler s = [] {
      sampler s;
      s->set_clamp(goop::wrap_mode::repeat);
      s->set_min_filter(goop::sampler_filter::linear, goop::sampler_filter::linear);
      s->set_mag_filter(goop::sampler_filter::linear);
      s->set_max_anisotropy(16);
      return s;
    }();

    // Todo: remove gl calls
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    rnu::vec2 const actual_size(w + 2 * (_info.outer_smoothness - _info.border_offset), h + 2 * (_info.outer_smoothness - _info.border_offset));

    glEnable(GL_SCISSOR_TEST);
    glViewport(x + _margin.x, y + _margin.y, actual_size.x, actual_size.y);
    glScissor(x, y, actual_size.x + _margin.x + _margin.z, actual_size.y + _margin.y + _margin.w);
    /*glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);*/
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    pipeline->bind(state);
    _atlas->bind(state, 0);
    s->bind(state, 0);
    if (_last_resolution != actual_size)
    {
      _last_resolution = actual_size;
      _info_dirty = true;
      _info.resolution = _last_resolution;
    }
    if (std::exchange(_info_dirty, false))
    {
      _block_info->write(_info);
    }
    _block_info->bind(state, 0);
    state.set_depth_test(false);

    geo->use_buffer(state, 0, vtb);
    geo->use_buffer(state, 1, _instances);
    geo->use_index_buffer(state, attribute_format::bit_width::x32, idb);
    geo->draw(state, primitive_type::triangles, { 6, std::uint32_t(_num_instances), 0, 0, 0 });
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    glDisable(GL_SCISSOR_TEST);
  }

  void sdf_2d::draw(draw_state_base& state, int x, int y)
  {
    draw(state, x, y, std::ceil(_size.x * _info.scale), std::ceil(_size.y * _info.scale));
  }

  void sdf_2d::set_scale(float scale)
  {
    set_info(&sdf_info::scale, scale);
  }

  static constexpr rnu::vec4ui8 norm_color(rnu::vec4 color)
  {
    return rnu::vec4ui8(rnu::clamp(color * 255, 0, 255));
  }

  void sdf_2d::set_color(rnu::vec4 color_rgba)
  {
    set_info(&sdf_info::color, norm_color(color_rgba));
  }
  void sdf_2d::set_border_color(rnu::vec4 border_color)
  {
    set_info(&sdf_info::border_color, norm_color(border_color));
  }
  void sdf_2d::set_border_width(float border_width)
  {
    set_info(&sdf_info::border_width, border_width);
  }
  void sdf_2d::set_border_offset(float border_offset)
  {
    set_info(&sdf_info::border_offset, border_offset);
  }
  void sdf_2d::set_outer_smoothness(float outer_smoothness)
  {
    set_info(&sdf_info::outer_smoothness, outer_smoothness);
  }
  void sdf_2d::set_inner_smoothness(float inner_smoothness)
  {
    set_info(&sdf_info::inner_smoothness, inner_smoothness);
  }
  rnu::vec2 sdf_2d::size() const
  {
    return _size * _info.scale + rnu::vec2(_margin.x, _margin.y) + rnu::vec2(_margin.z, _margin.w) +
      rnu::vec2(2 * (_info.outer_smoothness - _info.border_offset));
  }
  void sdf_2d::set_instances(std::span<sdf_instance const> instances)
  {
    _instances->load(instances);
    _num_instances = instances.size();
  }
  void sdf_2d::set_sdf_width(float width)
  {
    set_info(&sdf_info::sdf_width, width);
  }
  void sdf_2d::set_atlas(texture t)
  {
    _atlas = std::move(t);
  }
  void sdf_2d::set_default_size(rnu::vec2 size)
  {
    _size = size;
  }
}