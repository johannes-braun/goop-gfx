#pragma once

#include "sdf_font.hpp"

namespace goop::gui
{
  class text {
  public:
    void set_font(sdf_font font);
    void set_size(float size);
    void set_text(std::wstring_view text);

    void draw(draw_state_base& state, int x, int y, int w, int h);

  private:
    struct glyph_instance
    {
      rnu::vec2 offset;
      rnu::vec2 scale;
      rnu::vec2 uv_offset;
      rnu::vec2 uv_scale;
    };
    struct text_info {
      rnu::vec2 resolution;
      float font_scale;
    };

    std::optional<sdf_font> _font;
    float _font_size = 12;
    buffer _letter_instances;
    std::vector<glyph_instance> _glyphs;
    goop::mapped_buffer<text_info> text_block_info = { 1ull };
  };
}