#include "geometry.hpp"
#include <numeric>
#include <glad/glad.h>
#include "mapped_buffer.hpp"

namespace goop::opengl
{
  static constexpr GLuint attr_position = 0;
  static constexpr GLuint attr_normal = 1;
  static constexpr GLuint attr_uv0 = 2;
  static constexpr GLuint attr_uv1 = 3;
  static constexpr GLuint attr_uv2 = 4;
  static constexpr GLuint attr_color = 5;
  static constexpr GLuint attr_joints = 6;
  static constexpr GLuint attr_weights = 7;
  static constexpr GLuint buffer_binding = 0;

  geometry::~geometry()
  {
    if (glIsBuffer && glDeleteBuffers && glIsBuffer(_buffer))
      glDeleteBuffers(1, &_buffer);
    if (glIsVertexArray && glDeleteVertexArrays && glIsVertexArray(_buffer))
      glDeleteVertexArrays(1, &_vertex_array);
  }

  void geometry::upload(std::span<vertex const> vertices, std::span<geometry::index_type const> indices)
  {
    if(!glIsVertexArray(_vertex_array))
    {
      glCreateVertexArrays(1, &_vertex_array);

      glEnableVertexArrayAttrib(_vertex_array, attr_position);
      glVertexArrayAttribFormat(_vertex_array, attr_position, 3, GL_FLOAT, false, offsetof(vertex, position));

      glEnableVertexArrayAttrib(_vertex_array, attr_normal);
      glVertexArrayAttribFormat(_vertex_array, attr_normal, 3, GL_FLOAT, false, offsetof(vertex, normal));

      glEnableVertexArrayAttrib(_vertex_array, attr_uv0);
      glVertexArrayAttribFormat(_vertex_array, attr_uv0, 2, GL_FLOAT, false, offsetof(vertex, uv[0]));

      glEnableVertexArrayAttrib(_vertex_array, attr_uv1);
      glVertexArrayAttribFormat(_vertex_array, attr_uv1, 2, GL_FLOAT, false, offsetof(vertex, uv[1]));

      glEnableVertexArrayAttrib(_vertex_array, attr_uv2);
      glVertexArrayAttribFormat(_vertex_array, attr_uv2, 2, GL_FLOAT, false, offsetof(vertex, uv[2]));

      glEnableVertexArrayAttrib(_vertex_array, attr_color);
      glVertexArrayAttribFormat(_vertex_array, attr_color, 4, GL_UNSIGNED_BYTE, true, offsetof(vertex, color));

      glEnableVertexArrayAttrib(_vertex_array, attr_weights);
      glVertexArrayAttribFormat(_vertex_array, attr_weights, 4, GL_FLOAT, false, offsetof(vertex, weights));

      glEnableVertexArrayAttrib(_vertex_array, attr_joints);
      glVertexArrayAttribIFormat(_vertex_array, attr_joints, 4, GL_UNSIGNED_SHORT, offsetof(vertex, joints));

      glVertexArrayAttribBinding(_vertex_array, attr_color, buffer_binding);
      glVertexArrayAttribBinding(_vertex_array, attr_normal, buffer_binding);
      glVertexArrayAttribBinding(_vertex_array, attr_uv0, buffer_binding);
      glVertexArrayAttribBinding(_vertex_array, attr_uv1, buffer_binding);
      glVertexArrayAttribBinding(_vertex_array, attr_uv2, buffer_binding);
      glVertexArrayAttribBinding(_vertex_array, attr_color, buffer_binding);
      glVertexArrayAttribBinding(_vertex_array, attr_weights, buffer_binding);
      glVertexArrayAttribBinding(_vertex_array, attr_joints, buffer_binding);
      glVertexArrayBindingDivisor(_vertex_array, buffer_binding, 0);
    }

    auto const required_size = vertices.size_bytes() + indices.size_bytes();
    bool create_new = true;

    if (glIsBuffer(_buffer))
    {
      create_new = false;
      GLint64 size = 0;
      glGetNamedBufferParameteri64v(_buffer, GL_BUFFER_SIZE, &size);

      // Delete previous buffer because it has a static size, which also is too small.
      if (size < required_size)
      {
        glDeleteBuffers(1, &_buffer);
        create_new = true;
      }
      // else We can re-use this buffer because its larger than we need.
    }
    
    if(create_new)
    {
      glCreateBuffers(1, &_buffer);
      glNamedBufferStorage(_buffer, required_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    }

    glNamedBufferSubData(_buffer, 0, indices.size_bytes(), indices.data());
    glNamedBufferSubData(_buffer, indices.size_bytes(), vertices.size_bytes(), vertices.data());

    glVertexArrayVertexBuffer(_vertex_array, buffer_binding, _buffer, indices.size_bytes(), sizeof(vertex));
    glVertexArrayElementBuffer(_vertex_array, _buffer);
  }

  void geometry::draw_ranges(std::span<vertex_offset const> offsets)
  {
    glBindVertexArray(_vertex_array);
    
    _indirects.clear();
    for (auto const& offset : offsets)
      _indirects.push_back(make_indirect(offset));

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, _indirects.data(), _indirects.size(), sizeof(indirect));
  }

  void geometry::draw_ranges(mapped_buffer_base<indirect> const& indirects, size_t count, ptrdiff_t first)
  {
    glBindVertexArray(_vertex_array);
    auto const& gl_buffer = dynamic_cast<mapped_buffer<indirect> const&>(indirects);
    glBindBufferBase(GL_DRAW_INDIRECT_BUFFER, 0, gl_buffer.handle());
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, reinterpret_cast<void const*>(first), count, sizeof(indirect));
  }
}