#pragma once

#include <string_view>
#include <string>
#include <memory>
#include "draw_state.hpp"
#include "../generic/shader.hpp"
#include "handle.hpp"
#include <span>
#include <glad/glad.h>

namespace goop::opengl
{
  class shader : public shader_base, public single_handle
  {
  public:
    friend class shader_pipeline;

    ~shader();

    GLbitfield mask() const;

  protected:
    // Inherited via shader_base
    virtual bool compile_glsl(shader_type type, std::string_view source, std::string* info_log = nullptr) override;

  private:
    GLbitfield _mask = 0;
  };
  
  class shader_pipeline : public shader_pipeline_base, public single_handle
  {
  public:
    shader_pipeline();
    ~shader_pipeline();

    // Inherited via shader_pipeline_base
    virtual void use(shader_base const& shader) override;
    virtual void bind(draw_state_base& state) override;
  };
}