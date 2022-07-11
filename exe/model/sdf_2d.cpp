#include "sdf_2d.hpp"
#include <iostream>

namespace goop::gui
{
	constexpr auto vv = R"(#version 450 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 size;
layout(location = 2) in vec2 offset;
layout(location = 3) in vec2 uv_size;
layout(location = 4) in vec2 uv_offset;

layout(binding = 0) buffer TextInfo
{
  vec2 resolution;
  float font_scale;
};

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(location = 0) out vec2 uv;

void main()
{
  vec2 ncoord = 2 * ((font_scale * (position * size + offset)) / resolution) - 1;
  uv = position * uv_size + uv_offset;
  gl_Position = vec4(ncoord, 0.5, 1);
}
)";

	constexpr auto ff = R"(#version 450 core

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D atlas;

layout(binding = 0) buffer TextInfo
{
  vec2 resolution;
  float font_scale;
};

void main()
{
  float a = texture(atlas, uv).r;
  float b = 0;  

  float smoothness = 0.02 / font_scale;
  float half_smoothness = smoothness / 2.0;

  float border = 0.00 / font_scale;
  float half_border = border / 2.0;
  float border_offset = -0.005 / font_scale; 

  b = smoothstep(0.5 + border_offset + half_border - half_smoothness, 0.5 + border_offset + half_border + half_smoothness, a);
  a = smoothstep(0.5 + border_offset - half_border - half_smoothness, 0.5 + border_offset - half_border + half_smoothness, a);

  color = vec4(mix(vec3(0.1), vec3(1), border == 0.0 ? 1.0 : b), a);
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

        glEnable(GL_SCISSOR_TEST);
        glViewport(x + 8, y + 8, w - 16, h - 16);
        glScissor(x, y, w, h);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        pipeline->bind(state);
        _atlas->bind(state, 0);
        s->bind(state, 0);
        _block_info->write(sdf_info{ rnu::vec2(w - 16, h - 16), _scale });
        _block_info->bind(state, 0);
        state.set_depth_test(false);

        geo->use_buffer(state, 0, vtb);
        geo->use_buffer(state, 1, _instances);
        geo->use_index_buffer(state, attribute_format::bit_width::x32, idb);
        geo->draw(state, primitive_type::triangles, { 6, std::uint32_t(_num_instances), 0, 0, 0});
        glViewport(vp[0], vp[1], vp[2], vp[3]);
        glDisable(GL_SCISSOR_TEST);
	}

    void sdf_2d::draw(draw_state_base& state, int x, int y)
    {
        draw(state, x, y, std::ceil(_size.x * _scale), std::ceil(_size.y * _scale));
    }

    void sdf_2d::set_scale(float scale)
    {
        _scale = scale;
    }
    void sdf_2d::set_instances(std::span<sdf_instance const> instances)
    {
        _instances->load(instances);
        _num_instances = instances.size();
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