#include "geometry_format.hpp"
#include "buffer.hpp"

namespace goop::opengl
{

  constexpr int component_count(attribute_format::component_count count)
  {
    switch (count)
    {
    case attribute_format::component_count::scalar:
      return 1;
    case attribute_format::component_count::vec2:
      return 2;
    case attribute_format::component_count::vec3:
      return 3;
    case attribute_format::component_count::vec4:
      return 4;
    default:
      return 0;
    }
  }

  constexpr GLenum determine_type(attribute_format::data_type type, attribute_format::bit_width bits)
  {
#define declare_switch(B8, B16, B32, B64) \
    switch (bits) \
    { \
    case attribute_format::bit_width::x8: \
      return B8; \
    case attribute_format::bit_width::x16: \
      return B16; \
    case attribute_format::bit_width::x32: \
      return B32; \
    case attribute_format::bit_width::x64: \
      return B64; \
    }

    switch (type)
    {
    case attribute_format::data_type::f:
      declare_switch(GL_INVALID_ENUM, GL_HALF_FLOAT, GL_FLOAT, GL_DOUBLE);
      break;
    case attribute_format::data_type::i:
      declare_switch(GL_BYTE, GL_SHORT, GL_INT, GL_INVALID_ENUM);
      break;
    case attribute_format::data_type::u:
      declare_switch(GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT, GL_INVALID_ENUM);
      break;
    default:
      break;
    }
    return GL_INVALID_ENUM;
#undef declare_switchs
  }

  constexpr GLenum determine_mode(primitive_type type)
  {
    switch (type)
    {
    case primitive_type::points:
      return GL_POINTS;
    case primitive_type::lines:
      return GL_LINES;
    case primitive_type::line_strip:
      return GL_LINE_STRIP;
    case primitive_type::triangles:
      return GL_TRIANGLES;
    case primitive_type::triangle_strip:
      return GL_TRIANGLE_STRIP;
    default:
      return GL_INVALID_ENUM;
    }
  }

  geometry_format::geometry_format()
  {
    glCreateVertexArrays(1, &handle());
  }

  geometry_format::~geometry_format()
  {
    if (glIsVertexArray(handle()))
      glDeleteVertexArrays(1, &handle());
  }

  void geometry_format::set_attribute(std::size_t index, std::optional<attribute> attr)
  {
    if (!attr)
    {
      glDisableVertexArrayAttrib(handle(), index);
      return;
    }

    attribute const& a = *attr;
    auto const type = determine_type(a.format.type, a.format.bits);
    auto const components = component_count(a.format.components);
    glEnableVertexArrayAttrib(handle(), index);
    glVertexArrayAttribBinding(handle(), index, a.binding);

    if (a.format.type == attribute_format::data_type::f &&
      a.format.bits == attribute_format::bit_width::x64)
    {
      glVertexArrayAttribLFormat(handle(), index, components, type, a.structure_offset);
      return;
    }

    if ((a.format.type == attribute_format::data_type::i || a.format.type == attribute_format::data_type::u) &&
      !a.format.normalized)
    {
      glVertexArrayAttribIFormat(handle(), index, components, type, a.structure_offset);
      return;
    }

    glVertexArrayAttribFormat(handle(), index, components, type, a.format.type != attribute_format::data_type::f, a.structure_offset);
  }
  void geometry_format::use_index_buffer(draw_state_base& state, attribute_format::bit_width bits, handle_ref<buffer_base> buffer)
  {
    auto const& buf = buffer.as<opengl::buffer>();
    _index_type = determine_type(attribute_format::data_type::u, bits);
    glVertexArrayElementBuffer(handle(), buf->handle());
  }
  void geometry_format::use_buffer(draw_state_base& state, std::size_t binding, handle_ref<buffer_base> buffer, std::ptrdiff_t offset)
  {
    auto const& buf = buffer.as<opengl::buffer>();
    auto const id = buf->handle();
    glVertexArrayVertexBuffer(handle(), binding, id, offset, _strides[binding]);
  }
  void geometry_format::set_binding(std::size_t binding, std::size_t stride, attribute_repetition repeat)
  {
    _strides.resize(std::max(_strides.size(), binding + 1));
    _strides[binding] = stride;
    glVertexArrayBindingDivisor(handle(), binding, repeat == attribute_repetition::per_instance ? 1 : 0);
  }
  void geometry_format::draw_indexed(draw_state_base& state, primitive_type type, draw_info_indexed const& info)
  {
    glBindVertexArray(handle());
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glDrawElementsIndirect(determine_mode(type), _index_type, &info);
  }
  void geometry_format::draw_indexed(draw_state_base& state, primitive_type type, buffer_base const& indirect_buffer, std::size_t count, std::ptrdiff_t offset)
  {
    glBindVertexArray(handle());
    auto const buf = static_cast<buffer const&>(indirect_buffer).handle();
    void const* offset_ptr = nullptr;
    std::memcpy(&offset_ptr, &offset, sizeof(offset));
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf);
    glMultiDrawElementsIndirect(determine_mode(type), _index_type, offset_ptr, count, sizeof(draw_info_indexed));
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
  }
  void geometry_format::draw_array(draw_state_base& state, primitive_type type, draw_info_array const& info)
  {
    glBindVertexArray(handle());
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glDrawArraysIndirect(determine_mode(type), &info);
  }
  void geometry_format::draw_array(draw_state_base& state, primitive_type type, buffer_base const& indirect_buffer, std::size_t count, std::ptrdiff_t offset)
  {
    glBindVertexArray(handle());
    auto const buf = static_cast<buffer const&>(indirect_buffer).handle();
    void const* offset_ptr = nullptr;
    std::memcpy(&offset_ptr, &offset, sizeof(offset));
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buf);
    glMultiDrawArraysIndirect(determine_mode(type), offset_ptr, count, sizeof(draw_info_array));
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
  }
}