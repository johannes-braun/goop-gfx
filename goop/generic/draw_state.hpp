#pragma once

#include <memory>
#include <utility>
#include <optional>
#include <GLFW/glfw3.h>
#include <rnu/math/math.hpp>

namespace goop
{
  enum class culling_mode
  {
    none,
    back,
    front
  };

  enum class blending
  {
    zero,
    one,
    src_color,
    dst_color,
    src_alpha,
    dst_alpha,
    constant_color,
    constant_alpha
  };

  enum class blending_equation
  {
    src_plus_dst,
    src_minus_dst,
    dst_minus_src,
    min,
    max
  };

  struct blending_factor
  {
    bool invert = false;
    blending factor = blending::one;
  };

  struct blending_mode
  {
    blending_factor src_color{ false, blending::one };
    blending_factor dst_color{ false, blending::zero };
    blending_factor src_alpha{ false, blending::one };
    blending_factor dst_alpha{ false, blending::zero };
    blending_equation equation_color = blending_equation::src_plus_dst;
    blending_equation equation_alpha = blending_equation::src_plus_dst;
    std::optional<rnu::vec4> constant_color;
  };

  class render_target_base;
  class draw_state_base
  {
  public:
    virtual void from_window(GLFWwindow* window) = 0;
    virtual std::pair<int, int> current_surface_size() const = 0;
    virtual int display(render_target_base const& source, int source_width, int source_height) = 0;
    virtual void set_viewport(rnu::rect2f viewport) = 0;
    virtual void set_scissor(std::optional<rnu::rect2f> scissor) = 0;
    virtual void set_depth_test(bool enabled) = 0;
    virtual void set_culling_mode(culling_mode mode) = 0;
    virtual void set_blending(blending_mode const& blending) = 0;
    virtual void set_blending(std::nullopt_t) = 0;
  };
}