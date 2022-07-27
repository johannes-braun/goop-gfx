#pragma once

#include <concepts>
#include <rnu/math/math.hpp>

namespace goop::gui
{
  static constexpr rnu::vec4ui8 norm_color(rnu::vec4 color)
  {
    return rnu::vec4ui8(rnu::clamp(color * 255, 0, 255));
  }

  template<typename T>
  class dirty_info
  {
  public:
    using info_type = T;
    
    bool dirty() const {
      return _dirty;
    }

    T const& info() const {
      return _info;
    }

    void clear() { _dirty = false; }

    template<typename R, std::convertible_to<R> V>
    void set(R info_type::* var, V&& value) requires requires(V v) {
      { (v != v).any() };
    }
    {
      auto& last = _info.*var;
      if ((last != value).any())
      {
        last = value;
        _dirty = true;
      }
    }

    template<typename R, std::convertible_to<R> V>
    void set(R info_type::* var, V&& value)
    {
      auto& last = _info.*var;
      if (last != value)
      {
        last = value;
        _dirty = true;
      }
    }


  private:
    T _info;
    bool _dirty = true;
  };
}