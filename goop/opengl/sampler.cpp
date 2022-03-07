#include "sampler.hpp"
#include <glad/glad.h>

namespace goop::opengl
{
  constexpr GLuint make_mag_filter(sampler_filter filter)
  {
    return 0x2600 + static_cast<GLuint>(filter);
  }

  constexpr GLuint make_min_filter(sampler_filter filter, std::optional<sampler_filter> mipmap_filter = std::nullopt)
  {
    return 0x2600 + static_cast<GLuint>(filter) + (mipmap_filter ? (0x0100 + (static_cast<GLuint>(mipmap_filter.value()) << 1)) : 0u);
  }

  constexpr GLuint make_clamp(wrap_mode mode)
  {
    switch (mode)
    {
    case wrap_mode::clamp_to_edge:
      return GL_CLAMP_TO_EDGE;
    case wrap_mode::repeat:
      return GL_REPEAT;
    case wrap_mode::mirrored_repeat:
      return GL_MIRRORED_REPEAT;
    default:
      return GL_INVALID_VALUE;
    }
  }

  constexpr GLuint make_compare(compare cmp)
  {
    switch (cmp)
    {
    case compare::leq:
      return GL_LEQUAL;
    case compare::geq:
      return GL_GEQUAL;
    case compare::less:
      return GL_LESS;
    case compare::greater:
      return GL_GREATER;
    case compare::equal:
      return GL_EQUAL;
    case compare::not_equal:
      return GL_NOTEQUAL;
    case compare::always:
      return GL_ALWAYS;
    case compare::never:
      return GL_NEVER;
    default:
      return GL_INVALID_ENUM;
    }
  }

  sampler::sampler()
  {
    glCreateSamplers(1, &handle());
  }

  void sampler::bind(draw_state_base& state, std::uint32_t binding_point)
  {
    glBindSampler(binding_point, handle());
  }

  void sampler::set_compare_fun(compare cmp)
  {
    glSamplerParameteri(handle(), GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glSamplerParameteri(handle(), GL_TEXTURE_COMPARE_FUNC, make_compare(cmp));
  }

  sampler::~sampler()
  {
    if (glIsSampler && glDeleteSamplers && glIsSampler(handle()))
      glDeleteSamplers(1, &handle());
  }

  void sampler::set_clamp(wrap_mode r, wrap_mode s, wrap_mode t)
  {
    glSamplerParameteri(handle(), GL_TEXTURE_WRAP_R, make_clamp(r));
    glSamplerParameteri(handle(), GL_TEXTURE_WRAP_S, make_clamp(s));
    glSamplerParameteri(handle(), GL_TEXTURE_WRAP_T, make_clamp(t));
  }
  
  void sampler::set_clamp(wrap_mode rst)
  {
    set_clamp(rst, rst, rst);
  }

  void sampler::set_mag_filter(sampler_filter filter)
  {
    glSamplerParameteri(handle(), GL_TEXTURE_MAG_FILTER, make_mag_filter(filter));
  }

  void sampler::set_min_filter(sampler_filter filter, std::optional<sampler_filter> mipmap_filter)
  {
    glSamplerParameteri(handle(), GL_TEXTURE_MIN_FILTER, make_min_filter(filter, mipmap_filter));
  }

  void sampler::set_max_anisotropy(float a)
  {
    glSamplerParameterf(handle(), GL_TEXTURE_MAX_ANISOTROPY, a);
  }
}
