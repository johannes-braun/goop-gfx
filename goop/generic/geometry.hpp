#pragma once

#include <span>
#include <rnu/math/math.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include "draw_state.hpp"
#include "mapped_buffer.hpp"

namespace goop
{
  struct vertex
  {
    rnu::vec3 position;
    rnu::vec3 normal;
    rnu::vec2 uv[3];
    rnu::vec4ui8 color;
    rnu::vec4ui16 joints;
    rnu::vec4 weights;
  };

  struct vertex_offset
  {
    std::ptrdiff_t vertex_offset;
    std::size_t vertex_count;
    std::ptrdiff_t index_offset;
    std::size_t index_count;
  };

  struct indirect
  {
    std::uint32_t count;
    std::uint32_t prim_count;
    std::uint32_t first_index;
    std::uint32_t base_vertex;
    std::uint32_t base_instance;
  };

  static constexpr indirect make_indirect(vertex_offset const& offset)
  {
    return indirect{
      .count = static_cast<std::uint32_t>(offset.index_count),
      .prim_count = 1,
      .first_index = static_cast<std::uint32_t>(offset.index_offset),
      .base_vertex = static_cast<std::uint32_t>(offset.vertex_offset),
      .base_instance = 0
    };
  }

  class geometry_base
  {
  public:
    using index_type = std::uint32_t;

    geometry_base() = default;
    geometry_base(geometry_base const&) = delete;
    geometry_base& operator=(geometry_base const&) = delete;
    geometry_base(geometry_base&&) noexcept = default;
    geometry_base& operator=(geometry_base&&) noexcept = default;
    ~geometry_base();

    void clear();
    void free_client_memory();
    vertex_offset append_vertices(std::span<vertex const> vertices, std::span<index_type const> indices = {});
    vertex_offset append_empty_vertices(std::size_t vertex_count, std::size_t index_count = 0);

    void draw(draw_state_base& state, vertex_offset offset);
    void draw(draw_state_base& state, std::span<vertex_offset const> offsets);
    void draw(draw_state_base& state, mapped_buffer_base<indirect> const& indirects, size_t count, ptrdiff_t first = 0);
    void prepare();

  protected:
    virtual void upload(std::span<vertex const> vertices, std::span<geometry_base::index_type const> indices) = 0;
    virtual void draw_ranges(std::span<vertex_offset const> offsets) = 0;
    virtual void draw_ranges(mapped_buffer_base<indirect> const& indirects, size_t count, ptrdiff_t first = 0) = 0;

    mutable bool _dirty = false;
    mutable std::mutex _data_mutex;
    std::vector<vertex> _staging_vertices;
    std::vector<index_type> _staging_indices;
  };
}