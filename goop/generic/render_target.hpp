#pragma once

#include "texture.hpp"
#include "draw_state.hpp"
#include <span>

namespace goop
{
  class render_target_base
  {
  public:
    virtual void bind_texture_layer(int binding_point, texture_base const& texture, int level = 0, int layer = 0) = 0;
    virtual void bind_depth_stencil_texture_layer(texture_base const& texture, int level = 0, int layer = 0) = 0;
    virtual void bind_texture(int binding_point, texture_base const& texture, int level = 0) = 0;
    virtual void bind_depth_stencil_texture(texture_base const& texture, int level = 0) = 0;

    virtual void activate(draw_state_base& state) = 0;
    virtual void clear_color(int binding_point, std::span<float const, 4> color) = 0;
    virtual void clear_depth_stencil(float depth, int stencil) = 0;
    virtual void deactivate(draw_state_base& state) = 0;
  };
}