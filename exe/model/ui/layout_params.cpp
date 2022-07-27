#include "layout_params.hpp"

namespace goop::ui
{
  std::variant<int, layout_mode> get_mode(std::string_view str)
  {
    if (str == "match_parent")
      return layout_mode::match_parent;
    else if (str == "wrap_content")
      return layout_mode::wrap_content;
    else
    {
      int size = 0;
      auto const result = std::from_chars(str.data(), str.data() + str.size(), size);
      if (result.ec == std::errc::invalid_argument)
        return 0;
      return size;
    }
  }
  layout_params to_layout(std::string_view str, std::any additional_data)
  {
    auto const split_at = str.find_first_of(',');
    if (split_at == std::string_view::npos || split_at + 1 == str.size())
    {
      auto const mode = get_mode(str);
      return { mode, mode, additional_data };
    }

    auto const substr_first = trimmed(str.substr(0, split_at));
    auto const substr_second = trimmed(str.substr(split_at + 1));
    return { get_mode(substr_first), get_mode(substr_second), additional_data };
  }
}