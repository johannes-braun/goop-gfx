#include "element.hpp"

namespace goop::ui
{
  element_base::element_base(handle_ref<ui_context_base> context)
    : _context(std::move(context))
  {
  }
  handle_ref<ui_context_base> const& element_base::context() const
  {
    return _context;
  }
  handle_ref<ui_context_base>& element_base::context()
  {
    return _context;
  }
  void element_base::set_layout_params(layout_params params)
  {
    _layout_params = params;
    invalidate_layout();
  }
  layout_params const& element_base::get_layout_params()
  {
    return _layout_params;
  }
  void element_base::invalidate_draw()
  {
    _needs_redraw = true;
  }
  void element_base::invalidate_layout()
  {
    invalidate_draw();
    _needs_relayout = true;
  }
  bool element_base::needs_relayout() const
  {
    return _needs_relayout;
  }
  bool element_base::needs_redraw() const
  {
    return _needs_redraw;
  }
  void element_base::measure(layout_hint x_hint, layout_hint y_hint)
  {
    _last_x_hint = x_hint;
    _last_y_hint = y_hint;
    _measured_size = on_measure(x_hint, y_hint);
    set_measured_size(_measured_size);
  }
  rnu::vec4i element_base::margins() const
  {
    return _margin;
  }
  void element_base::set_margins(rnu::vec4 m)
  {
    _margin = m;
    invalidate_layout();
  }
  void element_base::layout(rnu::box<2, int> bounds)
  {
    auto const changed = _bounds != bounds;
    if (changed.any())
    {
      _bounds = bounds;
      _bounds.position += rnu::vec2i(_margin.x, _margin.y);

      _draw_buffer.set_position(_bounds.position);
      _draw_buffer.set_size(_bounds.size);
      _bounds.position = rnu::vec2i(0, 0);

      if (_draw_bitmap)
        context()->provider().free(*_draw_bitmap);

      _draw_bitmap = context()->provider().acquire(texture_type::t2d,
        data_type::rgba8unorm, _bounds.size.x, _bounds.size.y, 1);
      _draw_render_target->bind_texture(0, *_draw_bitmap, 0);

      _draw_buffer.set_color({ 1,1,1,1 });
      _draw_buffer.set_texture(_draw_bitmap);
      _draw_buffer.set_uv({ 0,0 }, { 1,1 });
    }
    on_layout(changed.any(), _bounds);

    _needs_relayout = false;
  }
  rnu::vec2i element_base::measured_size() const
  {
    return _measured_size;
  }
  rnu::vec2i element_base::measured_size_with_margins() const
  {
    return _measured_size + rnu::vec2i(_margin.x, _margin.y) + rnu::vec2i(_margin.z, _margin.w);
  }
  void element_base::render(draw_state_base& state, render_target& target)
  {
    // todo: if rebuild draw cache
    if (needs_redraw())
    {
      _draw_render_target->activate(state);
      _draw_render_target->clear_color(0, std::array{ 0.f,0.f,0.f,0.f });
      on_render(state, _draw_render_target);
    }

    target->activate(state);
    _draw_buffer.draw(state);
    target->deactivate(state);
  }
  rnu::vec2i element_base::desired_size() const {
    return { 0, 0 };
  }
  void element_base::on_layout(bool changed, rnu::box<2, int> bounds) {}
  rnu::vec2i element_base::on_measure(layout_hint x_hint, layout_hint y_hint)
  {
    auto const size = desired_size();
    auto const& [w, w_mode] = x_hint;
    auto const& [h, h_mode] = y_hint;

    int width = 0;
    int height = 0;

    if (w_mode == stretch_mode::exact)
      width = w;
    else if (w_mode == stretch_mode::at_most)
      width = std::min(size.x, w);
    else
      width = size.x;

    if (h_mode == stretch_mode::exact)
      height = h;
    else if (h_mode == stretch_mode::at_most)
      height = std::min(size.y, h);
    else
      height = size.y;

    return { width, height };
  }
  void element_base::on_render(draw_state_base& state, render_target& target) {}
  void element_base::set_measured_size(rnu::vec2i size)
  {
    _measured_size = size;
  }
}