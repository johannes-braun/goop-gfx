#pragma once

#include <glad/glad.h>

namespace goop::opengl
{
  class single_handle
  {
  public:
    GLuint const& handle() const;
    GLuint& handle();
    
  private:
    GLuint _handle = 0;
  };
}