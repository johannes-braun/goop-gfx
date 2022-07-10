#include "buffer.hpp"

namespace goop::opengl
{
  buffer::~buffer()
  {
    if (glIsBuffer(handle()))
      glDeleteBuffers(1, &handle());
  }

  std::size_t buffer::size() const
  {
    if (!glIsBuffer(handle()))
      return 0;

    GLint64 size = 0;
    glGetNamedBufferParameteri64v(handle(), GL_BUFFER_SIZE, &size);
    return size;
  }

  void buffer::load_impl(std::byte const* data, std::size_t data_size, std::ptrdiff_t offset)
  {
    if (!glIsBuffer(handle()))
    {
      glCreateBuffers(1, &handle());
      glNamedBufferStorage(handle(), data_size + offset, data, GL_DYNAMIC_STORAGE_BIT);
      return;
    }

    auto const s = size();
    if (s < data_size + offset)
    {
      GLuint new_handle = 0;
      glCreateBuffers(1, &new_handle);
      glNamedBufferStorage(new_handle, data_size + offset, nullptr, GL_DYNAMIC_STORAGE_BIT);
      glCopyNamedBufferSubData(handle(), new_handle, 0, 0, offset);
      auto last = std::exchange(handle(), new_handle);
      glDeleteBuffers(1, &last);
    }

    if (data != 0)
      glNamedBufferSubData(handle(), offset, data_size, data);
  }
}