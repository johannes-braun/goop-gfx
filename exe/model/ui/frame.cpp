#include "frame.hpp"

namespace goop::ui
{
  void frame_base::set_background_scaling(background_scaling scaling)
  {
    _background_scaling = scaling;
    invalidate_layout();
  }

  void frame_base::set_background(rnu::vec4 background)
  {
    _background.set_texture(std::nullopt);
    _background.set_color(background);
    _texture_size = { 0,0 };
    invalidate_draw();
  }

  void frame_base::set_background(texture background)
  {
    if (!_has_texture)
    {
      invalidate_layout();
      _has_texture = true;
    }
    _background.set_color(rnu::vec4(1, 1, 1, 1));
    _background.set_texture(background);
    _texture_size = background->dimensions();
    invalidate_draw();
  }

  rnu::vec2i frame_base::desired_size() const
  {
    return _texture_size;
  }

  void frame_base::on_layout(bool changed, rnu::box<2, int> bounds)
  {
    if (changed)
    {
      _background.set_position(bounds.position);
      _background.set_size(bounds.size);

      if (_has_texture)
      {
        auto const dst = bounds.size;
        auto const src = _texture_size;

        auto const dst_rel = dst.x / float(dst.y);
        auto const src_rel = src.x / float(src.y);
        switch (_background_scaling)
        {
        case background_scaling::stretch_fit:
        {
          // Fit inside, set uvs such that image fits fully inside of the bounds
          if (dst_rel > src_rel)
          {
            // limited by image height
            auto const height = 1.0;
            auto const width = dst_rel / src_rel;

            _background.set_uv({ 0.5f - width / 2, height / 2 + 0.5f }, { width / 2 + 0.5f, 0.5f - height / 2 });
          }
          else
          {
            // limited by image width
            auto const height = src_rel / dst_rel;
            auto const width = 1.0;

            _background.set_uv({ 0.5f - width / 2, height / 2 + 0.5f }, { width / 2 + 0.5f, 0.5f - height / 2 });
          }
        }
        break;
        case background_scaling::stretch_fill:
        {
          // Fit such that the image fills up the whole panel, centering it and cutting off overflows
          if (dst_rel > src_rel)
          {
            // limited by image height
            auto const height = src_rel / dst_rel;
            auto const width = 1.0;

            _background.set_uv({ 0.5f - width / 2, height / 2 + 0.5f }, { width / 2 + 0.5f, 0.5f - height / 2 });
          }
          else
          {
            // limited by image width
            auto const height = 1.0;
            auto const width = dst_rel / src_rel;

            _background.set_uv({ 0.5f - width / 2, height / 2 + 0.5f }, { width / 2 + 0.5f, 0.5f - height / 2 });
          }

        }
        break;
        }

        // Todo: background scaling...
      }
    }
  }

  void frame_base::on_render(draw_state_base& state, render_target& target)
  {
    _background.draw(state);
  }
}