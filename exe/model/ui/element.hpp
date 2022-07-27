#pragma once

#include "ui_context.hpp"
#include "layout_params.hpp"
#include "../panel_2d.hpp"

#include <generic/handle.hpp>

namespace goop::ui
{
  class element_base;
  using element = handle<element_base>;

  enum class stretch_mode
  {
    exact,
    at_most,
    undefined
  };
  enum class item_justify
  {
    start,
    middle,
    end
  };
  enum class orientation
  {
    horizontal,
    vertical
  };

  using layout_hint = std::pair<int, stretch_mode>;

  static constexpr layout_hint layout_exact(int value) { return { value, stretch_mode::exact }; };
  static constexpr layout_hint layout_at_most(int value) { return { value, stretch_mode::at_most }; };
  static constexpr layout_hint layout_undefined(int value) { return { value, stretch_mode::undefined }; };

  class element_base
  {
  public:
    element_base(handle_ref<ui_context_base> context);

    handle_ref<ui_context_base> const& context() const;
    handle_ref<ui_context_base>& context();

    void set_layout_params(layout_params params);
    layout_params const& get_layout_params();
    rnu::vec4i margins() const;
    void set_margins(rnu::vec4 m);

    void invalidate_draw();
    void invalidate_layout();
    virtual bool needs_relayout() const;
    virtual bool needs_redraw() const;

    rnu::vec2i measured_size() const;
    rnu::vec2i measured_size_with_margins() const;
    virtual rnu::vec2i desired_size() const;
    void measure(layout_hint x_hint, layout_hint y_hint);
    void layout(rnu::box<2, int> bounds);
    void render(draw_state_base& state, render_target& target);

  protected:
    virtual void on_layout(bool changed, rnu::box<2, int> bounds);
    virtual rnu::vec2i on_measure(layout_hint x_hint, layout_hint y_hint);
    virtual void on_render(draw_state_base& state, render_target& target);;

  private:
    void set_measured_size(rnu::vec2i size);

    layout_hint _last_x_hint;
    layout_hint _last_y_hint;
    bool _needs_relayout = true;
    bool _needs_redraw = true;
    layout_params _layout_params;
    rnu::vec2i _measured_size{ 0, 0 };
    rnu::box<2, int> _bounds{};
    rnu::vec4i _margin{};

    render_target _draw_render_target;
    std::optional<texture> _draw_bitmap;
    gui::panel_2d _draw_buffer;
    handle_ref<ui_context_base> _context;
  };
}