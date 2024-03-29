#include "multi_draw.hpp"

namespace goop
{
  void multi_draw_base::set_geometry(handle_ref<geometry_format_base> geometry)
  {
    _geometry = std::move(geometry);
  }
  void multi_draw_base::preallocate(size_t vertex_buffer_bytes, size_t index_buffer_bytes, size_t indirect_buffer_size)
  {
    _vertex_buffer->load(nullptr, vertex_buffer_bytes);
    _index_buffer->load(nullptr, index_buffer_bytes);
    if (indirect_buffer_size > 0)
      _indirect_buffer->load(nullptr, indirect_buffer_size);
  }
  void multi_draw_base::clear_queue()
  {
    _current_hash = 0;
    _draw_commands.clear();
  }
  void multi_draw_base::enqueue(draw_info_indexed const& info)
  {
    hash_combine(_current_hash, info.base_instance, info.base_vertex, info.count, info.first_index, info.instance_count);
    _draw_commands.push_back(info);
  }
  void multi_draw_base::draw(draw_state_base& state, primitive_type primitive)
  {
    if (!_geometry)
      return;

    if (_current_hash != _previous_hash)
    {
        _indirect_buffer->load(std::span(_draw_commands));
        _previous_hash = _current_hash;
    }

    _geometry->get().use_buffer(state, 0, _vertex_buffer);
    _geometry->get().use_index_buffer(state, _index_size.value(), _index_buffer);
    _geometry->get().draw_indexed(state, primitive, _indirect_buffer, _draw_commands.size());
  }
}