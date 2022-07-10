#pragma once

#include <span>
#include <rnu/math/math.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include "graphics.hpp"
#include "multi_draw.hpp"

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

  vertex mix(vertex const& lhs, vertex const& rhs, float t);

  struct vertex_offset
  {
    std::ptrdiff_t vertex_offset;
    std::size_t vertex_count;
    std::ptrdiff_t index_offset;
    std::size_t index_count;
  };

  enum class display_type
  {
    surfaces,
    outlines,
    vertices
  };

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

    void set_display_type(display_type type);
    display_type display_type() const;
    void clear();
    void free_client_memory();
    vertex_offset append_vertices(std::span<vertex const> vertices, std::span<index_type const> indices = {});
    vertex_offset append_empty_vertices(std::size_t vertex_count, std::size_t index_count = 0);

    void draw(draw_state_base& state, vertex_offset offset);
    void draw(draw_state_base& state, std::span<vertex_offset const> offsets);
    void prepare();

  protected:
    void upload(std::span<vertex const> vertices, std::span<geometry_base::index_type const> indices);
    void draw_ranges(draw_state_base& state, std::span<vertex_offset const> offsets);

    mutable bool _dirty = false;
    mutable std::mutex _data_mutex;
    goop::display_type _display_type;
    std::vector<vertex> _staging_vertices;
    std::vector<index_type> _staging_indices;

    std::optional<geometry_format> _geometry;
    multi_draw _drawer;
  };

  using geometry = handle<geometry_base, geometry_base>;
}