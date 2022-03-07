#include "handle.hpp"

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
}