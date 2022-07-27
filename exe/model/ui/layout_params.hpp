#pragma once

#include <variant>
#include <any>
#include <string_view>
#include <charconv>

namespace goop::ui
{
  enum class layout_mode
  {
    match_parent,
    wrap_content
  };

  struct layout_params
  {
    std::variant<int, layout_mode> width = layout_mode::wrap_content;
    std::variant<int, layout_mode> height = layout_mode::wrap_content;
    std::any additional_data;
  };

  constexpr std::string_view trimmed(std::string_view str)
  {
    auto const begin = str.find_first_not_of(" \n\t\r");
    auto const end = str.find_last_not_of(" \n\t\r");

    if (begin == std::string_view::npos)
      return str;
    if (end == std::string_view::npos)
      return str.substr(begin);
    return str.substr(begin, end + 1);
  }

  std::variant<int, layout_mode> get_mode(std::string_view str);
  layout_params to_layout(std::string_view str, std::any additional_data = {});
}