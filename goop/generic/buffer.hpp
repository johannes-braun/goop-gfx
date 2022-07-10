#pragma once

#include <cinttypes>
#include <cstddef>
#include <span>

namespace goop
{
  class buffer_base
  {
  public:
    template<typename T>
    void load(std::span<T> data, std::ptrdiff_t offset = 0);
    void load(std::byte const* data, std::size_t data_size, std::ptrdiff_t offset = 0);

    virtual std::size_t size() const = 0;

  protected:
    virtual void load_impl(std::byte const* data, std::size_t data_size, std::ptrdiff_t offset = 0) = 0;
  };

  template<typename T>
  void buffer_base::load(std::span<T> data, std::ptrdiff_t offset)
  {
    load(reinterpret_cast<std::byte const*>(data.data()), data.size_bytes(), offset);
  }
}