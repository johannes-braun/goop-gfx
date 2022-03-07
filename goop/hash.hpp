#pragma once

#include <utility>

namespace goop
{
  template <typename T, typename... Rest>
  inline void hash_combine(std::size_t& seed, T&& v, Rest&&... rest) {
    seed ^= std::hash<std::decay_t<T>>{}(std::forward<T>(v)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

    if constexpr (sizeof...(Rest) > 0)
      hash_combine(seed, std::forward<Rest>(rest)...);
  }

  template<typename... Ts>
  inline size_t hash(Ts&&... ts)
  {
    size_t seed = 0xc0ffee;
    hash_combine(seed, std::forward<Ts>(ts)...);
    return seed;
  }
}