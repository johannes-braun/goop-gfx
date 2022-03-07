#include "render_target.hpp"
#include "texture.hpp"

namespace goop::opengl
{
  render_target::render_target()
  {
    glCreateFramebuffers(1, &handle());
  }
  render_target::~render_target()
  {
    if (glIsFramebuffer && glDeleteFramebuffers && glIsFramebuffer(handle()))
      glDeleteFramebuffers(1, &handle());
  }
  void render_target::bind_texture_layer(int binding_point, texture_base const& texture, int level, int layer)
  {
    auto const base_texture = dynamic_cast<opengl::texture const&>(texture);
    glNamedFramebufferTextureLayer(handle(), GL_COLOR_ATTACHMENT0 + binding_point, base_texture.handle(), level, layer);
  }

  void render_target::bind_depth_stencil_texture_layer(texture_base const& texture, int level, int layer)
  {
    auto const& base_texture = dynamic_cast<opengl::texture const&>(texture);
    glNamedFramebufferTextureLayer(handle(), GL_DEPTH_STENCIL_ATTACHMENT, base_texture.handle(), level, layer);
  }
  void render_target::bind_texture(int binding_point, texture_base const& texture, int level)
  {
    auto const& base_texture = dynamic_cast<opengl::texture const&>(texture);
    glNamedFramebufferTexture(handle(), GL_COLOR_ATTACHMENT0 + binding_point, base_texture.handle(), level);
  }

  void render_target::bind_depth_stencil_texture(texture_base const& texture, int level)
  {
    auto const& base_texture = dynamic_cast<opengl::texture const&>(texture);
    glNamedFramebufferTexture(handle(), GL_DEPTH_STENCIL_ATTACHMENT, base_texture.handle(), level);
  }

  void render_target::activate(draw_state_base& state)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, handle());
  }

  void render_target::clear_color(int binding_point, std::span<float const, 4> color)
  {
    glClearNamedFramebufferfv(handle(), GL_COLOR, binding_point, color.data());
  }

  void render_target::clear_depth_stencil(float depth, int stencil)
  {
    glClearNamedFramebufferfi(handle(), GL_DEPTH_STENCIL, 0, depth, stencil);
  }

  void render_target::deactivate(draw_state_base& state)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}