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

  float border = info.border_width / info.scale / sdf_width / 2;
  float half_border = border / 2.0;
  float border_offset = -info.border_offset / info.scale / sdf_width; 

  b = smoothstep(0.5 + border_offset + half_border - half_smoothness_inner, 0.5 + border_offset + half_border + half_smoothness_inner, a);
  a = smoothstep(0.5 + border_offset - half_border - half_smoothness_outer, 0.5 + border_offset - half_border + half_smoothness_outer, a);

  vec4 inner_color = unpackUnorm4x8(info.color);
  vec4 border_color = unpackUnorm4x8(info.border_color);
  
  vec4 mixed_color = mix(border_color, inner_color, border == 0.0 ? 1.0 : b);

  color = vec4(mixed_color.rgb, mixed_color.a * a);
  //color.rgb *= color.a;
}
)";

  void sdf_2d::draw(draw_state_base& state, int x, int y, int w, int h)
  {
    if (_info.info().color == rnu::vec4(0, 0, 0, 0))
      return;

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

    rnu::rect2f const viewport{
      .position = {x + _margin.x, y + _margin.y},
      .size = {w + 2 * (_info.info().outer_smoothness - _info.info().border_offset), h + 2 * (_info.info().outer_smoothness - _info.info().border_offset)}
    };
    rnu::rect2f const scissor{
      .position = {x, y},
      .size = {viewport.size.x + _margin.x + _margin.z, viewport.size.y + _margin.y + _margin.w}
    };


    auto const [sw, sh] = state.current_surface_size();
    auto posv = viewport.position;
    //posv.y = sh - posv.y - viewport.size.y;
    auto poss = scissor.position;
    //poss.y = sh - poss.y - scissor.size.y;


    state.set_viewport({ posv, viewport.size });
    state.set_scissor(rnu::rect2f{ poss, scissor.size });
    state.set_culling_mode(culling_mode::none);
    /**/state.set_blending(blending_mode{
      .src_color = {false, blending::src_alpha},
      .dst_color = {true, blending::src_alpha},
      .src_alpha = {false, blending::one},
      .dst_alpha = {true, blending::src_alpha}, // 0 + dst * src
      .equation_color = blending_equation::src_plus_dst,
      .equation_alpha = blending_equation::src_plus_dst
      });

    pipeline->bind(state);
    _atlas->bind(state, 0);
    s->bind(state, 0);

    _info.set(&sdf_info::resolution, viewport.size);

    if (_info.dirty())
    {
      _block_info->write(_info.info());
      _info.clear();
    }
    _block_info->bind(state, 0);
    state.set_depth_test(false);

    geo->use_buffer(state, 0, vtb);
    geo->use_buffer(state, 1, _instances);
    geo->use_index_buffer(state, attribute_format::bit_width::x32, idb);
    geo->draw_indexed(state, primitive_type::triangles, { 6, std::uint32_t(_num_instances), 0, 0, 0 });
    
    state.set_scissor(std::nullopt);
    state.set_blending(std::nullopt);
  }

  void sdf_2d::draw(draw_state_base& state, int x, int y)
  {
    draw(state, x, y, std::ceil(_size.x * _info.info().scale), std::ceil(_size.y * _info.info().scale));
  }

  void sdf_2d::set_scale(float scale)
  {
    _info.set(&sdf_info::scale, scale);
  }

  void sdf_2d::set_color(rnu::vec4 color_rgba)
  {
    _info.set(&sdf_info::color, norm_color(color_rgba));
  }
  void sdf_2d::set_border_color(rnu::vec4 border_color)
  {
    _info.set(&sdf_info::border_color, norm_color(border_color));
  }
  void sdf_2d::set_border_width(float border_width)
  {
    _info.set(&sdf_info::border_width, border_width);
  }
  void sdf_2d::set_border_offset(float border_offset)
  {
    _info.set(&sdf_info::border_offset, border_offset);
  }
  void sdf_2d::set_outer_smoothness(float outer_smoothness)
  {
    _info.set(&sdf_info::outer_smoothness, outer_smoothness);
  }
  void sdf_2d::set_inner_smoothness(float inner_smoothness)
  {
    _info.set(&sdf_info::inner_smoothness, inner_smoothness);
  }
  rnu::vec2 sdf_2d::size() const
  {
    return _size * _info.info().scale + rnu::vec2(_margin.x, _margin.y) + rnu::vec2(_margin.z, _margin.w) +
      rnu::vec2(2 * (_info.info().outer_smoothness - _info.info().border_offset));
  }
  void sdf_2d::set_instances(std::span<sdf_instance const> instances)
  {
    _instances->load(instances);
    _num_instances = instances.size();
  }
  void sdf_2d::set_sdf_width(float width)
  {
    _info.set(&sdf_info::sdf_width, width);
  }
  void sdf_2d::set_atlas(texture t)
  {
    _atlas = std::move(t);
  }
  void sdf_2d::set_default_size(rnu::vec2 size)
  {
    _size = size;
  }

  float sdf_2d::scale() const
  {
    return _info.info().scale;
  }
}