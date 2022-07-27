#include "text.hpp"
#include "graphics.hpp"
#include <iostream>

namespace goop::gui
{

  void text::set_font(sdf_font font)
  {
    _font = std::move(font);
    set_atlas(_font.value()->atlas_texture());
    set_sdf_width(_font.value()->sdf_width());
  }

  void text::set_size(float size)
  {
    set_scale(_font.value()->em_factor(size));
  }

  void text::set_text(std::wstring_view text)
  {
    _glyphs.clear();
    int num_lines = 0;
    float x_max = 0;
    for (auto const& g : _font.value()->text_set(text, &num_lines, &x_max))
    {
      auto& v = _glyphs.emplace_back();

      v.offset = g.bounds.position;
      v.offset.y += ((num_lines - 1) * _font.value()->line_height()) - 
        _font.value()->font().descent() * _font.value()->base_size() / _font.value()->font().units_per_em();
      v.scale = g.bounds.size;
      v.uv_offset = g.uvs.position;
      v.uv_scale = g.uvs.size;
    }

    auto y_size = ((num_lines - 1) * _font.value()->line_height()) +
      _font.value()->font().ascent() * _font.value()->base_size() / _font.value()->font().units_per_em();

    set_default_size({ x_max, y_size});
    set_instances(std::span(_glyphs));
  }
}