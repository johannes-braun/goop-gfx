#pragma once

#include "graphics.hpp"

namespace goop
{
  class shadow
  {
  public:
    shadow(int width, int height, texture_provider_base<texture>& provider)
    {
      _depth_texture = provider.acquire(texture_type::t2d, data_type::d32fs8, width, height, 1);
      _target->bind_depth_stencil_texture(_depth_texture);
    }

    void begin(draw_state& state, rnu::mat4 const& matrix)
    {
      _target->clear_depth_stencil(1.f, 0);
      _target->activate(state);
    }

    texture end(draw_state& state)
    {
      _target->deactivate(state);
      return _depth_texture;
    }

  private:
    render_target _target;
    texture _depth_texture;
    texture_provider_base<texture> _provider;
  };
}