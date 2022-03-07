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

  void draw_state::set_viewport(float x, float y, float width, float height)
  {
    glViewportIndexedf(0, x, y, width, height);
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