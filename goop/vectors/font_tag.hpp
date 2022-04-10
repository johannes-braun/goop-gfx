#pragma once

#include <cstdint>

namespace goop
{
  enum class font_tag : std::uint32_t {};

  consteval std::uint32_t make_u32tag(char const (&arr)[5])
  {
    return std::uint32_t(arr[3] | (arr[2] << 8) | (arr[1] << 16) | (arr[0] << 24));
  }

  consteval font_tag make_tag(char const (&arr)[5])
  {
    return font_tag{ make_u32tag(arr) };
  }
}