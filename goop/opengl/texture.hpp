#pragma once

#include "../generic/texture.hpp"
#include "handle.hpp"
#include <glad/glad.h>

namespace goop::opengl
{
  class texture : public texture_base, public single_handle
  {
  public:
    ~texture() override;

    // Inherited via texture_base
    virtual void allocate(texture_type type, data_type data, int w, int mipmap_levels) override;
    virtual void allocate(texture_type type, data_type data, int w, int h, int mipmap_levels_or_samples) override;
    virtual void allocate(texture_type type, data_type data, int w, int h, int d, int mipmap_levels_or_samples) override;
    
    virtual void set_data(int level, int xoff, int w, int components, std::span<std::uint8_t const> pixel_data) override;
    virtual void set_data(int level, int xoff, int w, int components, std::span<float const> pixel_data) override;
    virtual void set_data(int level, int xoff, int yoff, int w, int h, int components, std::span<std::uint8_t const> pixel_data) override;
    virtual void set_data(int level, int xoff, int yoff, int w, int h, int components, std::span<float const> pixel_data) override;
    virtual void set_data(int level, int xoff, int yoff, int zoff, int w, int h, int d, int components, std::span<std::uint8_t const> pixel_data) override;
    virtual void set_data(int level, int xoff, int yoff, int zoff, int w, int h, int d, int components, std::span<float const> pixel_data) override;

    virtual void generate_mipmaps() override;
    virtual void bind(draw_state_base& state, std::uint32_t binding_point) const override;
    virtual rnu::vec3i dimensions() const override;

  private:
    rnu::vec3i _dimensions{};
  };
}