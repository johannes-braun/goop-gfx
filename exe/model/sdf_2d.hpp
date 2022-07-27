#pragma once

#include <rnu/math/math.hpp>
#include <optional>
#include <vector>
#include "graphics.hpp"
#include "dirty_info.hpp"

namespace goop::gui
{
  class sdf_2d
  {
  public:
    void draw(draw_state_base& state, int x, int y, int w, int h);
    void draw(draw_state_base& state, int x, int y);
    void set_scale(float scale);
    void set_color(rnu::vec4 color_rgba);
    void set_border_color(rnu::vec4 border_color);
    void set_border_width(float border_width);
    void set_border_offset(float border_offset);
    void set_outer_smoothness(float outer_smoothness);
    void set_inner_smoothness(float inner_smoothness);

    float scale() const;
    rnu::vec2 size() const;

  protected:
    struct sdf_instance
    {
      rnu::vec2 offset;
      rnu::vec2 scale;
      rnu::vec2 uv_offset;
      rnu::vec2 uv_scale;
    };
    struct sdf_info {
      rnu::vec2 resolution = {};
      float scale = 1.0;
      float border_width = 0;

      rnu::vec4ui8 color = { 255, 255, 255, 255 };
      rnu::vec4ui8 border_color = { 0, 0, 0, 255 };
      float border_offset = 0.3;
      float outer_smoothness = 1;

      float inner_smoothness = 0.707;
      float sdf_width = 0;
    };

    void set_instances(std::span<sdf_instance const> instances);
    void set_sdf_width(float width);
    void set_atlas(texture t);
    void set_default_size(rnu::vec2 size);

  private:
    dirty_info<sdf_info> _info;

    rnu::vec4i _margin = rnu::vec4i(0,0,0,0);
    size_t _num_instances = 0;
    texture _atlas;
    buffer _instances;
    rnu::vec2 _last_resolution = {0,0};
    rnu::vec2 _size;
    std::vector<sdf_instance> _glyphs;
    goop::mapped_buffer<sdf_info> _block_info = { 1ull };
  };
}