#pragma once

#include "../generic/geometry_format.hpp"
#include "handle.hpp"
#include <vector>

namespace goop::opengl
{
  class geometry_format : public geometry_format_base, public single_handle
  {
  public:
    geometry_format();

    ~geometry_format();

    void set_attribute(std::size_t index, std::optional<attribute> attr) override;
    void use_index_buffer(draw_state_base& state, attribute_format::bit_width bits, handle_ref<buffer_base> buffer) override;
    void use_buffer(draw_state_base& state, std::size_t binding, handle_ref<buffer_base> buffer, std::ptrdiff_t offset) override;
    void set_binding(std::size_t binding, std::size_t stride, attribute_repetition repeat) override;
    void draw(draw_state_base& state, primitive_type type, draw_info const& info) override;
    void draw(draw_state_base& state, primitive_type type, buffer_base const& indirect_buffer, std::size_t count, std::ptrdiff_t offset) override;

  private:
    std::vector<std::size_t> _strides;
    GLenum _index_type = 0;
  };
}