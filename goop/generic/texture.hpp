#pragma once

#include <memory>
#include <span>
#include "draw_state.hpp"

namespace goop
{
  enum class texture_type
  {
    t1d,
    t1d_array,
    t2d,
    t2d_array,
    t2d_ms,
    t2d_ms_array,
    t3d
  };

  enum class data_type
  {
    r8unorm,
    rg8unorm,
    rgb8unorm,
    rgba8unorm,
    r16f,
    rg16f,
    rgb16f,
    rgba16f,
    r32f,
    rg32f,
    rgb32f,
    rgba32f,
    d24s8,
    d32fs8,
  };
  static constexpr auto compute_texture_mipmap_count = 0;

  class texture_base
  {
  public:
    virtual ~texture_base() = default;

    virtual void allocate(texture_type type, data_type data, int w, int mipmap_levels) = 0;
    virtual void allocate(texture_type type, data_type data, int w, int h, int mipmap_levels_or_samples) = 0;
    virtual void allocate(texture_type type, data_type data, int w, int h, int d, int mipmap_levels_or_samples) = 0;
    virtual void set_data(int level, int xoff, int w, int components, std::span<std::uint8_t const> pixel_data) = 0;
    virtual void set_data(int level, int xoff, int w, int components, std::span<float const> pixel_data) = 0;
    virtual void set_data(int level, int xoff, int yoff, int w, int h, int components, std::span<std::uint8_t const> pixel_data) = 0;
    virtual void set_data(int level, int xoff, int yoff, int w, int h, int components, std::span<float const> pixel_data) = 0;
    virtual void set_data(int level, int xoff, int yoff, int zoff, int w, int h, int d, int components, std::span<std::uint8_t const> pixel_data) = 0;
    virtual void set_data(int level, int xoff, int yoff, int zoff, int w, int h, int d, int components, std::span<float const> pixel_data) = 0;
    virtual void generate_mipmaps() = 0;
    virtual void bind(draw_state_base& state, std::uint32_t binding_point) const = 0;
  };
}