#include "default_app.hpp"

namespace goop
{
  static struct glfw_static_context
  {
    glfw_static_context() { glfwInit(); }
    ~glfw_static_context() { glfwTerminate(); }
  } const glfw_context;

	void default_app::window_deleter::operator()(GLFWwindow* window) const
	{
		glfwDestroyWindow(window);
	}

	default_app::default_app(std::string const& title)
		: _window(glfwCreateWindow(start_width, start_height, title.c_str(), nullptr, nullptr)),
		_draw_state(_window.get())
	{
    _last_frame_time = std::chrono::steady_clock::now();
	}

	default_app::~default_app()
	{
	}

  bool default_app::begin_frame()
  {
    if (glfwWindowShouldClose(_window.get()))
      return false;

    while(glfwGetWindowAttrib(_window.get(), GLFW_ICONIFIED))
    {
      glfwWaitEvents();
      if (glfwWindowShouldClose(_window.get()))
        return false;
    }

    auto const n = std::chrono::steady_clock::now();
    _current_delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(
      n - _last_frame_time);
    _last_frame_time = n;
    auto [window_width, window_height] = _draw_state->current_surface_size();

    if(!_override_controls)
      update_controls(_current_delta_time);

    std::array clear_color{ 0.5f, 0.7f, 1.0f, 1.0f };

    _draw_textures.color = _texture_provider.acquire(goop::texture_type::t2d_ms, goop::data_type::rgba16f, window_width, window_height, 4);
    _draw_textures.depth_stencil = _texture_provider.acquire(goop::texture_type::t2d_ms, goop::data_type::d24s8, window_width, window_height, 4);

    _draw_state->set_viewport(0, 0, window_width, window_height);
    _default_render_target->bind_texture(0, _draw_textures.color);
    _default_render_target->bind_depth_stencil_texture(_draw_textures.depth_stencil);
    _default_render_target->activate(_draw_state);
    _default_render_target->clear_color(0, clear_color);
    _default_render_target->clear_depth_stencil(1.f, 0);

    return true;
  }

	int default_app::end_frame()
	{
    auto [window_width, window_height] = _draw_state->current_surface_size();
    _default_render_target->deactivate(_draw_state);

    _texture_provider.free(_draw_textures.color);
    _texture_provider.free(_draw_textures.depth_stencil);

    auto const img = _draw_state->display(_default_render_target, window_width, window_height);
    glfwPollEvents();
    return img;
	}

  void default_app::update_controls(std::chrono::duration<double> dt)
  {
    double mouse_x, mouse_y;
    glfwGetCursorPos(_window.get(), &mouse_x, &mouse_y);
    if (!_mouse_down[GLFW_MOUSE_BUTTON_LEFT] && glfwGetMouseButton(_window.get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
      _mouse_down[GLFW_MOUSE_BUTTON_LEFT] = true;
      _camera_controls.mx = { float(mouse_x) };
      _camera_controls.my = { float(mouse_y) };
    }
    else if (_mouse_down[GLFW_MOUSE_BUTTON_LEFT] && glfwGetMouseButton(_window.get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
      _mouse_down[GLFW_MOUSE_BUTTON_LEFT] = false;
    }
    if (glfwGetMouseButton(_window.get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      _camera_controls.mx.to(mouse_x);
      _camera_controls.my.to(mouse_y);
    }

    _camera_controls.axis.to({ 16 * (glfwGetKey(_window.get(), GLFW_KEY_W) - glfwGetKey(_window.get(), GLFW_KEY_S)),
      16 * (glfwGetKey(_window.get(), GLFW_KEY_A) - glfwGetKey(_window.get(), GLFW_KEY_D)),
      16 * (glfwGetKey(_window.get(), GLFW_KEY_Q) - glfwGetKey(_window.get(), GLFW_KEY_E)) });

    _camera_controls.mx.update(8 * dt.count());
    _camera_controls.my.update(8 * dt.count());
    _camera_controls.axis.update(10 * dt.count());

    _default_camera.axis(dt.count(), _camera_controls.axis.value().x, _camera_controls.axis.value().y, _camera_controls.axis.value().z);
    _default_camera.mouse(_camera_controls.mx.value(), _camera_controls.my.value(), !_camera_controls.mx.finished() || !_camera_controls.my.finished());
  }

  std::chrono::duration<double> default_app::current_delta_time() const
  {
    return _current_delta_time;
  }
  default_app::window_ptr const& default_app::window() const {
    return _window;
  }
  draw_state& default_app::default_draw_state() {
    return _draw_state;
  }
  rnu::cameraf const& default_app::default_camera() const {
    return _default_camera;
  }
  texture_provider const& default_app::default_texture_provider() const {
    return _texture_provider;
  }
  texture_provider & default_app::default_texture_provider()  {
    return _texture_provider;
  }
  render_target const& default_app::default_render_target() const {
    return _default_render_target;
  }
  render_target & default_app::default_render_target() {
    return _default_render_target;
  }
}