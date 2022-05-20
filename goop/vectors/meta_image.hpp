#pragma once

#include <rnu/math/math.hpp>

namespace goop
{
  /// <summary>
  /// Anything that resembles a kind of image
  /// </summary>
  template<typename T, std::size_t C>
  class meta_image
  {
  public:
    static constexpr std::size_t components = C;
    using component_type = T;
    using value_type = rnu::vec<T, C>;



  private:
  };
}