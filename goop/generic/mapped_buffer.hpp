#pragma once
#include <span>
#include <memory>
#include "draw_state.hpp"

namespace goop
{
  template<typename T>
  class mapped_buffer_base
  {
  public:
    mapped_buffer_base() = default;
    ~mapped_buffer_base() = default;
    mapped_buffer_base(mapped_buffer_base const&) = delete;
    mapped_buffer_base& operator=(mapped_buffer_base const&) = delete;
    mapped_buffer_base(mapped_buffer_base&&) noexcept = default;
    mapped_buffer_base& operator=(mapped_buffer_base&&) noexcept = default;

    virtual void allocate(std::size_t elements) = 0;
    virtual void map() = 0;
    virtual void unmap() = 0;
    virtual void flush() = 0;
    virtual void bind(draw_state_base& state, std::uint32_t binding) const = 0;

    T& operator[](std::ptrdiff_t index);
    T const& operator[](std::ptrdiff_t index) const;
    T& at(std::ptrdiff_t index);
    T const& at(std::ptrdiff_t index) const;

    std::size_t size() const;
    std::size_t size_bytes() const;
    void* data();
    void const* data() const;

    void write(std::span<T const> items);
    void write(T const& item);

  protected:
    std::size_t _capacity = 0;
    std::size_t _size = 0;
    T* _mapped_ptr = nullptr;
  };

  template<typename T>
  T& mapped_buffer_base<T>::operator[](std::ptrdiff_t index)
  {
    return at(index);
  }

  template<typename T>
  T const& mapped_buffer_base<T>::operator[](std::ptrdiff_t index) const
  {
    return at(index);
  }

  template<typename T>
  T& mapped_buffer_base<T>::at(std::ptrdiff_t index)
  {
    return _mapped_ptr[index];
  }

  template<typename T>
  T const& mapped_buffer_base<T>::at(std::ptrdiff_t index) const
  {
    return _mapped_ptr[index];
  }

  template<typename T>
  std::size_t mapped_buffer_base<T>::size() const
  {
    return _size;
  }

  template<typename T>
  std::size_t mapped_buffer_base<T>::size_bytes() const
  {
    return size() * sizeof(T);
  }

  template<typename T>
  void* mapped_buffer_base<T>::data()
  {
    return _mapped_ptr;
  }

  template<typename T>
  void const* mapped_buffer_base<T>::data() const
  {
    return _mapped_ptr;
  }

  template<typename T>
  void mapped_buffer_base<T>::write(std::span<T const> items)
  {
    allocate(items.size());
    map();
    std::memcpy(data(), items.data(), items.size_bytes());
    flush();
    unmap();
  }

  template<typename T>
  void mapped_buffer_base<T>::write(T const& item)
  {
    write(std::span(&item, 1));
  }
}

#include "mapped_buffer.inl.hpp"