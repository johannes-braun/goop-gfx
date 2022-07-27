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
    void set_viewport(rnu::rect2f viewport) override;
    void set_scissor(std::optional<rnu::rect2f> scissor) override;
    void set_culling_mode(culling_mode mode) override;
    void set_blending(blending_mode const& blending) override;
    void set_blending(std::nullopt_t) override;

  private:
    int _image = 0;
    GLFWwindow* _window = nullptr;
  };
}