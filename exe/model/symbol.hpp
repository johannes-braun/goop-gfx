#pragma once

#include "sdf_2d.hpp"
#include "vector_graphics.hpp"

namespace goop::gui
{
    class symbol : public sdf_2d {
    public:
        void set_holder(vector_graphics_holder holder);
        void set_icon(std::size_t index);

    private:
        std::optional<vector_graphics_holder> _holder;
        sdf_instance _icon;
    };
}