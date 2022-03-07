#pragma once

#include <cstdint>
#include <cinttypes>
#include <string_view>
#include <string>
#include "draw_state.hpp"

namespace goop
{
  enum class shader_type
  {
    vertex,
    geometry,
    fragment,
    compute,
    tess_eval,
    tess_control
  };

  class shader_base
  {
  public:
    virtual ~shader_base() = default;
    bool load_source(shader_type type, std::string_view source, std::string* info_log = nullptr);
    std::size_t hash() const;

  protected:
    virtual bool compile_glsl(shader_type type, std::string_view source, std::string* info_log = nullptr) = 0;

  private:
    std::size_t _hash = 0;
  };

  class shader_pipeline_base
  {
  public:
    virtual ~shader_pipeline_base() = default;

    virtual void use(shader_base const& shader) = 0;
    virtual void bind(draw_state_base& state) = 0;

  };
}