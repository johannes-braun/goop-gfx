#pragma once

#include "frame.hpp"
#include "../sdf_font.hpp"
#include "../text.hpp"

namespace goop::ui
{
  class label_base : public frame_base
  {
  public:
    using frame_base::frame_base;

    void set_font(gui::sdf_font font);
    void set_color(rnu::vec4 color);
    void set_text(std::wstring_view text);
    void set_size(float size);

    rnu::vec2i desired_size() const override;
    void on_layout(bool changed, rnu::box<2, int> bounds) override;
    void on_render(draw_state_base& state, render_target& target) override;

  private:
    rnu::vec2 _draw_position;
    rnu::vec2 _draw_size;
    gui::text _text;
  };

  using label = handle<label_base>;
}