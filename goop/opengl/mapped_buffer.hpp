#pragma once
#include <span>
#include <memory>
#include "draw_state.hpp"
#include "../generic/mapped_buffer.hpp"
#include "handle.hpp"
#include <glad/glad.h>

namespace goop::opengl
{
  template<typename T> 
  class mapped_buffer : public mapped_buffer_base<T>, public single_handle
  {
  public:
    ~mapped_buffer();

    virtual void allocate(std::size_t elements) override;
    virtual void map() override;
    virtual void unmap() override;
    virtual void flush() override;
    virtual void bind(draw_state_base& state, std::uint32_t binding) const override;
  };

  template<typename T>
  mapped_buffer<T>::~mapped_buffer()
  {
    if (glIsBuffer && glDeleteBuffers && glIsBuffer(handle()))
      glDeleteBuffers(1, &handle());
  }

  template<typename T>
  inline void mapped_buffer<T>::allocate(std::size_t elements)
  {
    if (glIsBuffer(handle()))
    {
      if (mapped_buffer_base<T>::_capacity >= elements * sizeof(T))
      {
        mapped_buffer_base<T>::_size = elements;
        return;
      }
      glDeleteBuffers(1, &handle());
    }
    glCreateBuffers(1, &handle());
    mapped_buffer_base<T>::_capacity = elements;
    mapped_buffer_base<T>::_size = elements;
    glNamedBufferStorage(handle(), elements * sizeof(T), nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT | GL_CLIENT_STORAGE_BIT);
  }

  template<typename T>
  void mapped_buffer<T>::map()
  {
    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    mapped_buffer_base<T>::_mapped_ptr = static_cast<T*>(glMapNamedBufferRange(handle(), 0, mapped_buffer_base<T>::size() * sizeof(T), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
  }

  template<typename T>
  void mapped_buffer<T>::unmap()
  {
    glUnmapNamedBuffer(handle());
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
  }

  template<typename T>
  void mapped_buffer<T>::flush()
  {
    glFlushMappedNamedBufferRange(handle(), 0, mapped_buffer_base<T>::size() * sizeof(T));
  }

  template<typename T>
  void mapped_buffer<T>::bind(draw_state_base& state, std::uint32_t binding) const
  {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, handle());
  }
}
