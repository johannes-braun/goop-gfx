#pragma once

#include <memory>
#include <optional>
#include "draw_state.hpp"
#include "../generic/sampler.hpp"
#include "handle.hpp"

namespace goop::opengl
{
  class sampler : public sampler_base, public single_handle
  {
  public:
    sampler();
    ~sampler();

    // Inherited via sampler_base
    virtual void set_clamp(wrap_mode r, wrap_mode s, wrap_mode t) override;
    virtual void set_clamp(wrap_mode rst) override;
    virtual void set_mag_filter(sampler_filter filter) override;
    virtual void set_min_filter(sampler_filter filter, std::optional<sampler_filter> mipmap_filter = std::nullopt) override;
    virtual void set_max_anisotropy(float a) override;
    virtual void bind(draw_state_base& state, std::uint32_t binding_point) override;
    virtual void set_compare_fun(compare cmp) override;
  };
}