#include "label.hpp"

namespace goop::ui
{
 void label_base::set_font(gui::sdf_font font)
  {
    _text.set_font(font);
    invalidate_layout();
  }
 void label_base::set_color(rnu::vec4 color)
  {
    _text.set_color(color);
    invalidate_draw();
  }
 void label_base::set_text(std::wstring_view text)
  {
    _text.set_text(text);
    invalidate_layout();
  }
 void label_base::set_size(float size)
  {
    _text.set_size(size);
    invalidate_layout();
  }
 rnu::vec2i label_base::desired_size() const {
    return _text.size();
  }
 void label_base::on_layout(bool changed, rnu::box<2, int> bounds)
  {
    frame_base::on_layout(changed, bounds);
    if (changed)
    {
      _draw_position = bounds.position;
      _draw_size = bounds.size;
    }
  }
 void label_base::on_render(draw_state_base& state, render_target& target)
  {
    frame_base::on_render(state, target);
    _text.draw(state, _draw_position.x, _draw_position.y, _draw_size.x, _draw_size.y);
  }
}