#pragma once
#include "../generic/buffer.hpp"
#include "handle.hpp"

namespace goop::opengl 
{
  class buffer : public buffer_base, public single_handle
  {
  public:
    ~buffer();
    std::size_t size() const override;
    void load_impl(std::byte const* data, std::size_t data_size, std::ptrdiff_t offset = 0) override;
  };
}