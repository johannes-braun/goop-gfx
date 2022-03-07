#pragma once
#include "../generic/draw_state.hpp"

namespace goop::opengl
{   

  class draw_state : public draw_state_base
  {
  public:
    void from_window(GLFWwindow* window) override;
    std::pair<int, int> current_surface_size() const override;
    int display(render_target_base const& source, int source_width, int source_height) override;
    void set_depth_test(bool enabled) override;
    void set_viewport(float x, float y, float width, float height) override;

  private:
    int _image = 0;
    GLFWwindow* _window = nullptr;
  };
}