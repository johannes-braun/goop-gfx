#include "geometry.hpp"
#include <numeric>

namespace goop
{
  vertex mix(vertex const& lhs, vertex const& rhs, float t)
  {
    vertex v;
    v.position = lhs.position * (1 - t) + rhs.position * t;
    v.normal = normalize(lhs.normal + rhs.normal);
    for (int i = 0; i < std::size(v.uv); ++i)
      v.uv[i] = lhs.uv[i] * (1 - t) + rhs.uv[i] * t;
    v.color = lhs.color * (1 - t) + rhs.color * t;
    v.joints = t < 0.5 ? lhs.joints : rhs.joints;
    v.weights = t < 0.5 ? lhs.weights : rhs.weights;
    return v;
  }

  geometry_base::~geometry_base() = default;

  void geometry_base::clear()
  {
    std::unique_lock lock(_data_mutex);
    _staging_vertices.clear();
    _staging_indices.clear();
    _dirty = true;
  }

  void geometry_base::free_client_memory()
  {
    std::unique_lock lock(_data_mutex);
    _staging_vertices.clear();
    _staging_vertices.shrink_to_fit();
    _staging_indices.clear();
    _staging_indices.shrink_to_fit();
  }

  void geometry_base::set_display_type(goop::display_type type)
  {
    _display_type = type;
  }

  display_type geometry_base::display_type() const
  {
    return _display_type;
  }

  vertex_offset geometry_base::append_vertices(std::span<vertex const> vertices, std::span<index_type const> indices)
  {
    auto const offset = append_empty_vertices(vertices.size(), indices.size());

    std::unique_lock lock(_data_mutex);
    std::copy(begin(vertices), end(vertices), std::next(_staging_vertices.begin(), offset.vertex_offset));
    if(!indices.empty())
      std::copy(begin(indices), end(indices), std::next(_staging_indices.begin(), offset.index_offset));

    _dirty = true;

    return offset;
  }

  vertex_offset geometry_base::append_empty_vertices(std::size_t vertex_count, std::size_t index_count)
  {
    vertex_offset const offset{
      .vertex_offset = static_cast<ptrdiff_t>(_staging_vertices.size()),
      .vertex_count = vertex_count,
      .index_offset = static_cast<ptrdiff_t>(_staging_indices.size()),
      .index_count = index_count == 0 ? vertex_count : index_count
    };

    std::unique_lock lock(_data_mutex);
    _staging_indices.resize(_staging_indices.size() + offset.index_count);
    _staging_vertices.resize(_staging_vertices.size() + vertex_count);

    // If index count not specified or set to zero, assume a 1:1 relation between vertices and indices.
    if (index_count == 0)
    {
      std::iota(std::next(_staging_indices.begin(), offset.index_offset), _staging_indices.end(), /*static_cast<index_type>(offset.vertex_offset)*/0);
    }
    _dirty = true;

    return offset;
  }
  void geometry_base::draw(draw_state_base& state, vertex_offset offset)
  {
    // don't need this here.
    draw(state, std::initializer_list{ offset });
  }
  void geometry_base::draw(draw_state_base& state, std::span<vertex_offset const> offsets)
  {
    // don't need this here.
    (void)state;

    std::unique_lock lock(_data_mutex);
    if (_dirty)
      prepare();

    draw_ranges(offsets);
  }
  void geometry_base::draw(draw_state_base& state, mapped_buffer_base<indirect> const& indirects, size_t count, ptrdiff_t first)
  {
    (void)state;

    std::unique_lock lock(_data_mutex);
    if (_dirty)
      prepare();

    draw_ranges(indirects, count, first);
  }
  void geometry_base::prepare()
  {
    upload(_staging_vertices, _staging_indices);
    _dirty = false;
  }
}
