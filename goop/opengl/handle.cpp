#include "handle.hpp"
#include <utility>

namespace goop::opengl
{
  GLuint const& single_handle::handle() const
  {
    return _handle;
  }
  GLuint& single_handle::handle()
  {
    return _handle;
  }

  single_handle::single_handle(single_handle&& other) noexcept
    : _handle(std::exchange(other._handle, nullhandle))
  {
    
  }

  single_handle& single_handle::operator=(single_handle&& other) noexcept
  {
    _handle = std::exchange(other._handle, nullhandle);
    return *this;
  }
}