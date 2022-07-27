#pragma once

#include <rnu/math/math.hpp>
#include "dirty_info.hpp"
#include <graphics.hpp>

namespace goop::gui
{
  class panel_2d
  {
  public:
    void set_position(rnu::vec2 position);
    void set_size(rnu::vec2 size);
    void set_color(rnu::vec4 color);
    void set_uv(rnu::vec2 bottom_left, rnu::vec2 top_right);
    void set_texture(std::optional<texture> texture);

    void draw(draw_state_base& state);

  private:
    struct panel_info 
    {
      rnu::vec2 position;
      rnu::vec2 size;
      rnu::vec2 uv_bottom_left;
      rnu::vec2 uv_top_right;
      rnu::vec4ui8 color;
      std::uint32_t has_texture;
    };

    std::optional<texture> _texture;
    dirty_info<panel_info> _info;
    goop::mapped_buffer<panel_info> _info_buffer;
  };
}