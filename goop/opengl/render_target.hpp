#pragma once

#include "../generic/render_target.hpp"
#include "handle.hpp"

namespace goop::opengl
{
  class render_target : public render_target_base, public single_handle
  {
  public:
    render_target();
    ~render_target();
    void bind_texture(int binding_point, texture_base const& texture, int level) override;
    void bind_depth_stencil_texture(texture_base const& texture, int level) override;
    void bind_texture_layer(int binding_point, texture_base const& texture, int level, int layer) override;
    void bind_depth_stencil_texture_layer(texture_base const& texture, int level, int layer) override;

    void activate(draw_state_base& state) override;
    void clear_color(int binding_point, std::span<float const, 4> color) override;
    void clear_depth_stencil(float depth, int stencil) override;
    void deactivate(draw_state_base& state) override;

  private:
  };
}