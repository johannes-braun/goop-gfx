#include "texture.hpp"
#include <glad/glad.h>
#include <cmath>

namespace goop::opengl
{
  constexpr GLenum get_texture_type(texture_type type)
  {
    switch (type)
    {
    case texture_type::t1d:
      return GL_TEXTURE_1D;
    case texture_type::t1d_array:
      return GL_TEXTURE_1D_ARRAY;
    case texture_type::t2d:
      return GL_TEXTURE_2D;
    case texture_type::t2d_array:
      return GL_TEXTURE_2D_ARRAY;
    case texture_type::t2d_ms:
      return GL_TEXTURE_2D_MULTISAMPLE;
    case texture_type::t2d_ms_array:
      return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
    case texture_type::t3d:
      return GL_TEXTURE_3D;
    default:
      return GL_INVALID_VALUE;
    }
  }

  constexpr GLenum get_data_type(data_type type)
  {
    switch (type)
    {
    case data_type::r8unorm:
      return GL_R8;
    case data_type::rg8unorm:
      return GL_RG8;
    case data_type::rgb8unorm:
      return GL_RGB8;
    case data_type::rgba8unorm:
      return GL_RGBA8;
    case data_type::r16f:
      return GL_R16F;
    case data_type::rg16f:
      return GL_RG16F;
    case data_type::rgb16f:
      return GL_RGB16F;
    case data_type::rgba16f:
      return GL_RGBA16F;
    case data_type::r32f:
      return GL_R32F;
    case data_type::rg32f:
      return GL_RG32F;
    case data_type::rgb32f:
      return GL_RGB32F;
    case data_type::rgba32f:
      return GL_RGBA32F;
    case data_type::d24s8:
      return GL_DEPTH24_STENCIL8;
    case data_type::d32fs8:
      return GL_DEPTH32F_STENCIL8;
    default:
      return GL_INVALID_VALUE;
    }
  }

  constexpr GLenum get_format(int components)
  {
    switch (components)
    {
    case 1:
      return GL_RED;
    case 2:
      return GL_RG;
    case 3:
      return GL_RGB;
    case 4:
      return GL_RGBA;
    default:
      return GL_INVALID_VALUE;
    }
  }

  int num_mipmaps(int w, int h, int d)
  {
    return 1 + std::floor(std::log2(std::max(w, std::max(h, d))));
  }

  void texture::allocate(texture_type type, data_type data, int w, int mipmap_levels)
  {
    _dimensions = { w, 1, 1 };
    glCreateTextures(get_texture_type(type), 1, &handle());
    glTextureStorage1D(handle(), mipmap_levels == 0 ? num_mipmaps(w, 1, 1) : mipmap_levels, get_data_type(data), w);
  }

  void texture::allocate(texture_type type, data_type data, int w, int h, int mipmap_levels)
  {
    _dimensions = { w, h, 1 };
    glCreateTextures(get_texture_type(type), 1, &handle());
    if (type == texture_type::t2d_ms)
    {
      glTextureStorage2DMultisample(handle(), mipmap_levels, get_data_type(data), w, h, true);
    }
    else
    {
      glTextureStorage2D(handle(), mipmap_levels == 0 ? num_mipmaps(w, h, 1) : mipmap_levels, get_data_type(data), w, h);
    }
  }

  void texture::allocate(texture_type type, data_type data, int w, int h, int d, int mipmap_levels)
  {
    _dimensions = { w, h, d };
    glCreateTextures(get_texture_type(type), 1, &handle());
    if (type == texture_type::t2d_ms_array)
    {
      glTextureStorage3DMultisample(handle(), mipmap_levels, get_data_type(data), w, h, d, true);
    }
    else
    {
      glTextureStorage3D(handle(), mipmap_levels, get_data_type(data), w, h, d);
    }
  }

  texture::~texture()
  {
    if (glIsTexture && glDeleteTextures && glIsTexture(handle()))
      glDeleteTextures(1, &handle());
  }

  void texture::set_data(int level, int xoff, int w, int components, std::span<std::uint8_t const> pixel_data)
  {
    glTextureSubImage1D(handle(), level, xoff, w, get_format(components), GL_UNSIGNED_BYTE, pixel_data.data());
  }

  void texture::set_data(int level, int xoff, int yoff, int w, int h, int components, std::span<std::uint8_t const> pixel_data)
  {
    glTextureSubImage2D(handle(), level, xoff, yoff, w, h, get_format(components), GL_UNSIGNED_BYTE, pixel_data.data());
  }

  void texture::set_data(int level, int xoff, int yoff, int zoff, int w, int h, int d, int components, std::span<std::uint8_t const> pixel_data)
  {
    glTextureSubImage3D(handle(), level, xoff, yoff, zoff, w, h, d, get_format(components), GL_UNSIGNED_BYTE, pixel_data.data());
  }

  void texture::set_data(int level, int xoff, int w, int components, std::span<float const> pixel_data)
  {
    glTextureSubImage1D(handle(), level, xoff, w, get_format(components), GL_FLOAT, pixel_data.data());
  }

  void texture::set_data(int level, int xoff, int yoff, int w, int h, int components, std::span<float const> pixel_data)
  {
    glTextureSubImage2D(handle(), level, xoff, yoff, w, h, get_format(components), GL_FLOAT, pixel_data.data());
  }

  void texture::set_data(int level, int xoff, int yoff, int zoff, int w, int h, int d, int components, std::span<float const> pixel_data)
  {
    glTextureSubImage3D(handle(), level, xoff, yoff, zoff, w, h, d, get_format(components), GL_FLOAT, pixel_data.data());
  }

  void texture::generate_mipmaps()
  {
    glGenerateTextureMipmap(handle());
  }

  void texture::bind(draw_state_base& state, std::uint32_t binding_point) const
  {
    glBindTextureUnit(binding_point, handle());
  }

  rnu::vec3i texture::dimensions() const
  {
    return _dimensions;
  }
}
