#pragma once

#include <memory>
#include <optional>
#include "draw_state.hpp"

namespace goop
{
  enum class sampler_filter {
    nearest = 0x0,
    linear = 0x1,
  };

  enum class wrap_mode
  {
    clamp_to_edge,
    repeat,
    mirrored_repeat
  };

  enum class compare
  {
    leq,
    geq,
    less,
    greater,
    equal,
    not_equal,
    always,
    never
  };

  class sampler_base
  {
  public:
    virtual ~sampler_base() = default;

    virtual void set_clamp(wrap_mode r, wrap_mode s, wrap_mode t) = 0;
    virtual void set_clamp(wrap_mode rst) = 0;
    virtual void set_mag_filter(sampler_filter filter) = 0;
    virtual void set_min_filter(sampler_filter filter, std::optional<sampler_filter> mipmap_filter = std::nullopt) = 0;
    virtual void set_max_anisotropy(float a) = 0;
    virtual void set_compare_fun(compare cmp) = 0;
    virtual void bind(draw_state_base& state, std::uint32_t binding_point) = 0;
  };
}