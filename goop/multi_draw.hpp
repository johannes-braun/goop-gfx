#pragma once

#include "graphics.hpp"
#include <vector>
#include <span>
#include <stdexcept>

namespace goop
{
  class multi_draw_base
  {
  public:
    void set_geometry(handle_ref<geometry_format_base> geometry);
    void preallocate(size_t vertex_buffer_bytes, size_t index_buffer_bytes, size_t indirect_buffer_size = 0);

    void clear_append_cursor() {
      _vertex_count = 0;
      _index_count = 0;
    }

    template<typename T, std::integral I>
    draw_info append_data(std::span<T> data, std::span<I> indices)
    {
      auto const index_type = [&] {
        switch (sizeof(I))
        {
        case 1:
          return attribute_format::bit_width::x8;
        case 2:
          return attribute_format::bit_width::x16;
        case 4:
          return attribute_format::bit_width::x32;
        case 8:
          return attribute_format::bit_width::x64;
        default:
          return attribute_format::bit_width::x32;
        }
      }();

      if (_index_size && index_type != _index_size.value())
        throw std::invalid_argument("Index type is different from previously set one.");

      _index_size = index_type;

      draw_info info{
        .count = std::uint32_t(indices.size()),
        .instance_count = 1,
        .first_index = std::uint32_t(_index_count),
        .base_vertex = std::uint32_t(_vertex_count),
        .base_instance = 0
      };
      _vertex_buffer->load(data, _vertex_count * sizeof(T));
      _index_buffer->load(indices, _index_count * sizeof(I));
      _vertex_count += data.size();
      _index_count += indices.size();
      return info;
    }

    void clear_queue();
    void enqueue(draw_info const& info);
    void draw(draw_state_base& state, primitive_type primitive);

  private:
    std::optional<attribute_format::bit_width> _index_size;
    std::optional<handle_ref<geometry_format_base>> _geometry;
    buffer _vertex_buffer;
    buffer _index_buffer;
    buffer _indirect_buffer;
    size_t _vertex_count = 0;
    size_t _index_count = 0;
    std::size_t _previous_hash = 0;
    std::size_t _current_hash = 0;
    std::vector<draw_info> _draw_commands;
  };

  using multi_draw = handle<multi_draw_base, multi_draw_base>;
}