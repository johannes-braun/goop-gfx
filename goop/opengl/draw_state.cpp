#include "draw_state.hpp"
#include <glad/glad.h>
#include "render_target.hpp"

namespace goop::opengl
{
  std::pair<int, int> draw_state::current_surface_size() const
  {
    std::pair<int, int> size;
    glfwGetFramebufferSize(_window, &size.first, &size.second);
    return size;
  }

  int draw_state::display(render_target_base const& source, int source_width, int source_height)
  {
    auto const& gl_source = dynamic_cast<render_target const&>(source);
    auto const [window_width, window_height] = current_surface_size();
    glBlitNamedFramebuffer(gl_source.handle(), 0, 0, 0, source_width, source_height, 0, 0, window_width, window_height,
      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glfwSwapBuffers(_window);
    return _image = (_image + 1) % 2;
  }

  void draw_state::set_depth_test(bool enabled)
  {
    if (enabled)
      glEnable(GL_DEPTH_TEST);
    else
      glDisable(GL_DEPTH_TEST);
  }

  void draw_state::set_culling_mode(culling_mode mode)
  {
    switch (mode)
    {
    case culling_mode::none:
      glDisable(GL_CULL_FACE);
      break;
    case culling_mode::back:
      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
      break;
    case culling_mode::front:
      glEnable(GL_CULL_FACE);
      glCullFace(GL_FRONT);
      break;
    default:
      break;
    }
  }

  constexpr GLenum get_blending_factor(blending_factor const& factor)
  {
    auto const inv = factor.invert;
    switch (factor.factor)
    {
    case blending::zero: return inv ? GL_ONE : GL_ZERO;
    case blending::one: return inv ? GL_ZERO : GL_ONE;
    case blending::src_color: return inv ? GL_ONE_MINUS_SRC_COLOR : GL_SRC_COLOR;
    case blending::dst_color: return inv ? GL_ONE_MINUS_DST_COLOR : GL_DST_COLOR;
    case blending::src_alpha: return inv ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
    case blending::dst_alpha: return inv ? GL_ONE_MINUS_DST_ALPHA : GL_DST_ALPHA;
    case blending::constant_color: return inv ? GL_ONE_MINUS_CONSTANT_COLOR : GL_CONSTANT_COLOR;
    case blending::constant_alpha: return inv ? GL_ONE_MINUS_CONSTANT_ALPHA : GL_CONSTANT_ALPHA;
    }
    return GL_ZERO;
  }

  constexpr GLenum get_blending_equation(blending_equation eq)
  {
    switch (eq)
    {
    case blending_equation::src_plus_dst: return GL_FUNC_ADD;
    case blending_equation::src_minus_dst: return GL_FUNC_SUBTRACT;
    case blending_equation::dst_minus_src: return GL_FUNC_REVERSE_SUBTRACT;
    case blending_equation::min: return GL_MIN;
    case blending_equation::max: return GL_MAX;
    }
    return GL_FUNC_ADD;
  }

  void draw_state::set_blending(std::nullopt_t)
  {
    glDisable(GL_BLEND);
  }
  
  void draw_state::set_blending(blending_mode const& blending)
  {
    glEnable(GL_BLEND);
    glBlendFuncSeparate(
      get_blending_factor(blending.src_color), 
      get_blending_factor(blending.dst_color), 
      get_blending_factor(blending.src_alpha), 
      get_blending_factor(blending.dst_alpha));
    if (blending.constant_color)
    {
      glBlendColor(
        blending.constant_color->r,
        blending.constant_color->g,
        blending.constant_color->b,
        blending.constant_color->a);
    }
    glBlendEquationSeparate(get_blending_equation(blending.equation_color),
      get_blending_equation(blending.equation_alpha));
  }

  void draw_state::set_viewport(rnu::rect2f viewport)
  {
    glViewportIndexedf(0, viewport.position.x, viewport.position.y, viewport.size.x, viewport.size.y);
  }

  void draw_state::set_scissor(std::optional<rnu::rect2f> scissor)
  {
    if (scissor)
    {
      glEnable(GL_SCISSOR_TEST);
      glScissorIndexed(0, scissor->position.x, scissor->position.y, scissor->size.x, scissor->size.y);
    }
    else
    {
      glDisable(GL_SCISSOR_TEST);
    }
  }

  void draw_state::from_window(GLFWwindow* window)
  {
    _window = window;
    glfwMakeContextCurrent(_window);
    gladLoadGLLoader(GLADloadproc(&glfwGetProcAddress));

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
  }

}