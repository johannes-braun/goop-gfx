#pragma once

#include <span>
#include <rnu/math/math.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include "draw_state.hpp"
#include "../generic/geometry.hpp"
#include <glad/glad.h>

namespace goop::opengl
{
  class geometry : public geometry_base
  {
  public:
    ~geometry();

  protected:
    // Inherited via geometry_base
    virtual void upload(std::span<vertex const> vertices, std::span<geometry_base::index_type const> indices) override;
    virtual void draw_ranges(std::span<vertex_offset const> offsets) override;
    virtual void draw_ranges(mapped_buffer_base<indirect> const& indirects, size_t count, ptrdiff_t first) override;

  private:
    GLuint _vertex_array = 0;
    GLuint _buffer = 0;
    std::vector<indirect> _indirects;
  };
}