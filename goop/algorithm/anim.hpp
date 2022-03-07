#pragma once

#include <rnu/math/math.hpp>

namespace goop {
  template<typename T>
  struct smooth
  {
    constexpr smooth() : smooth(0) {}
    constexpr smooth(T val) : _val(val), _dst(val) {}

    constexpr void to(T dst) { _dst = dst; }

    constexpr void update(double delta_s)
    {
      _val += (_dst - _val) * delta_s;
    }

    constexpr bool finished() {
      return rnu::abs(_dst - _val) < 0.001f;
    }

    constexpr T& value() {
      return _val;
    }

    constexpr T const& value() const {
      return _val;
    }

  private:
    T _val;
    T _dst;
  };
}