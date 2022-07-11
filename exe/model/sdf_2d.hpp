#pragma once

#include <rnu/math/math.hpp>
#include <optional>
#include <vector>
#include "graphics.hpp"

namespace goop::gui
{
    class sdf_2d
    {
    public:
        void draw(draw_state_base& state, int x, int y, int w, int h);
        void draw(draw_state_base& state, int x, int y);
        void set_scale(float scale);

    protected:
        struct sdf_instance
        {
            rnu::vec2 offset;
            rnu::vec2 scale;
            rnu::vec2 uv_offset;
            rnu::vec2 uv_scale;
        };
        struct sdf_info {
            rnu::vec2 resolution;
            float font_scale;
        };

        void set_instances(std::span<sdf_instance const> instances);
        void set_atlas(texture t);
        void set_default_size(rnu::vec2 size);

    private:
        float _scale = 1.0f;
        size_t _num_instances = 0;
        texture _atlas;
        buffer _instances;
        rnu::vec2 _size;
        std::vector<sdf_instance> _glyphs;
        goop::mapped_buffer<sdf_info> _block_info = { 1ull };
    };
}