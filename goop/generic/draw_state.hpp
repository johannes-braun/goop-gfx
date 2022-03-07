#pragma once

#include <memory>
#include <utility>
#include <GLFW/glfw3.h>

namespace goop
{
  class render_target_base; 
  class draw_state_base
  {
  public:
    virtual void from_window(GLFWwindow* window) = 0;
    virtual std::pair<int, int> current_surface_size() const = 0;
    virtual int display(render_target_base const& source, int source_width, int source_height) = 0;
    virtual void set_viewport(float x, float y, float width, float height) = 0;

    virtual void set_depth_test(bool enabled) = 0;
  };
}