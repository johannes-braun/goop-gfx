#include "text.hpp"
#include "graphics.hpp"
#include <iostream>

namespace goop::gui
{

  void text::set_font(sdf_font font)
  {
    _font = std::move(font);
    set_atlas(_font.value()->atlas_texture());
  }

  void text::set_size(float size)
  {
    set_scale(_font.value()->em_factor(size));
  }

  void text::set_text(std::wstring_view text)
  {
    _glyphs.clear();
    int num_lines = 0;
    rnu::vec2 min{ std::numeric_limits<float>::max() };
    rnu::vec2 max{ std::numeric_limits<float>::lowest() };
    for (auto const& g : _font.value()->text_set(text, &num_lines))
    {
      auto& v = _glyphs.emplace_back();

      v.offset = g.bounds.position;
      v.offset.y += ((num_lines - 1) * _font.value()->line_height()) - _font.value()->font().descent() * _font.value()->base_size() / _font.value()->font().units_per_em();
      v.scale = g.bounds.size;
      v.uv_offset = g.uvs.position;
      v.uv_scale = g.uvs.size;

      min = rnu::min(v.offset, min);
      max = rnu::max(v.offset + v.scale, max);
    }
    set_default_size(max - min);
    set_instances(std::span(_glyphs));
  }
}