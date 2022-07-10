#pragma once

#include <glad/glad.h>

namespace goop::opengl
{
  class single_handle
  {
  public:
    static constexpr GLuint nullhandle = 0;

    single_handle() = default;
    single_handle(single_handle const&) = delete;
    single_handle& operator=(single_handle const&) = delete;
    single_handle(single_handle&& other) noexcept;
    single_handle& operator=(single_handle&& other) noexcept;

    GLuint const& handle() const;
    GLuint& handle();
    
  private:
    GLuint _handle = nullhandle;
  };
}