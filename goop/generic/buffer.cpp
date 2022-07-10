#include "buffer.hpp"
#include "buffer.hpp"
#include "buffer.hpp"
#include "buffer.hpp"
#include "buffer.hpp"
#include "buffer.hpp"
#include "buffer.hpp"

namespace goop
{
  void buffer_base::load(std::byte const* data, std::size_t data_size, std::ptrdiff_t offset)
  {
    load_impl(data, data_size, offset);
  }
}

