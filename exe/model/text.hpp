#pragma once

#include "sdf_font.hpp"
#include "sdf_2d.hpp"

namespace goop::gui
{
  class text : public sdf_2d {
  public:
    void set_font(sdf_font font);
    void set_size(float size);
    void set_text(std::wstring_view text);

  private:
    std::optional<sdf_font> _font;
    std::vector<sdf_instance> _glyphs;
  };
}