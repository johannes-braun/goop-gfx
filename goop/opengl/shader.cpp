#include "shader.hpp"
#include <glad/glad.h>

namespace goop::opengl
{
  constexpr GLenum shader_type_of(shader_type type)
  {
    switch (type)
    {
    case shader_type::vertex:
      return GL_VERTEX_SHADER;
    case shader_type::geometry:
      return GL_GEOMETRY_SHADER;
    case shader_type::fragment:
      return GL_FRAGMENT_SHADER;
    case shader_type::compute:
      return GL_COMPUTE_SHADER;
    case shader_type::tess_eval:
      return GL_TESS_EVALUATION_SHADER;
    case shader_type::tess_control:
      return GL_TESS_CONTROL_SHADER;
    default:
      return GL_INVALID_ENUM;
    }
  }

  constexpr GLenum shader_mask_of(shader_type type)
  {
    switch (type)
    {
    case shader_type::vertex:
      return GL_VERTEX_SHADER_BIT;
    case shader_type::geometry:
      return GL_GEOMETRY_SHADER_BIT;
    case shader_type::fragment:
      return GL_FRAGMENT_SHADER_BIT;
    case shader_type::compute:
      return GL_COMPUTE_SHADER_BIT;
    case shader_type::tess_eval:
      return GL_TESS_EVALUATION_SHADER_BIT;
    case shader_type::tess_control:
      return GL_TESS_CONTROL_SHADER_BIT;
    default:
      return 0;
    }
  }

  shader::~shader() {
    if (glDeleteProgram && glIsProgram && glIsProgram(handle()))
      glDeleteProgram(handle());
  }

  GLbitfield shader::mask() const
  {
    return _mask;
  }

  bool shader::compile_glsl(shader_type type, std::string_view source, std::string* info_log)
  {
    if (glIsProgram(handle()))
      glDeleteProgram(handle());

    GLchar const* const raw = source.data();
    handle() = glCreateShaderProgramv(shader_type_of(type), 1, &raw);
    _mask = shader_mask_of(type);

    GLint link_status = false;
    glGetProgramiv(handle(), GL_LINK_STATUS, &link_status);

    if (info_log)
    {
      GLint len = 0;
      glGetProgramiv(handle(), GL_INFO_LOG_LENGTH, &len);
      info_log->resize(len);
      glGetProgramInfoLog(handle(), len, &len, info_log->data());
    }

    if (!link_status)
    {
      glDeleteProgram(handle());
      handle() = 0;
      _mask = 0;
    }

    return link_status;
  }

  shader_pipeline::shader_pipeline()
  {
    glCreateProgramPipelines(1, &handle());
  }


  void shader_pipeline::use(shader_base const& shader)
  {
    auto const& gl_shader = dynamic_cast<opengl::shader const&>(shader);
    glUseProgramStages(handle(), gl_shader.mask(), gl_shader.handle());
  }

  void shader_pipeline::bind(draw_state_base& state)
  {
    glBindProgramPipeline(handle());
  }

  shader_pipeline::~shader_pipeline()
  {
    if (glDeleteProgramPipelines && glIsProgramPipeline && glIsProgramPipeline(handle()))
      glDeleteProgramPipelines(1, &handle());
  }
}

