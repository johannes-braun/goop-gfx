#pragma once

#include <graphics.hpp>

namespace goop::ui
{
  class ui_context_base
  {
  public:
    ui_context_base(texture_provider& provider)
    {
      _provider = &provider;
    }

    texture_provider const& provider() const { return *_provider; }
    texture_provider& provider() { return *_provider; }

  private:
    texture_provider* _provider = nullptr;
  };

  using ui_context = handle<ui_context_base>;
}