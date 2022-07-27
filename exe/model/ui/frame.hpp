#pragma once

#include "element.hpp"

namespace goop::ui
{
  class frame_base : public element_base
  {
  public:
    using element_base::element_base;

    enum class background_scaling
    {
      stretch_fit,
      stretch_fill,
    };

    void set_background_scaling(background_scaling scaling);
    void set_background(rnu::vec4 background);
    void set_background(texture background);

  protected:
    rnu::vec2i desired_size() const override;
    void on_layout(bool changed, rnu::box<2, int> bounds) override;
    void on_render(draw_state_base& state, render_target& target) override;

  private:
    bool _has_texture = false;
    rnu::vec2i _texture_size;
    gui::panel_2d _background;
    background_scaling _background_scaling = background_scaling::stretch_fit;
  };

  using frame = handle<frame_base>;
}
