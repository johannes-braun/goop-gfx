#pragma once

#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <rnu/camera.hpp>
#include <graphics.hpp>
#include <algorithm/anim.hpp>

namespace goop
{
	class default_app
	{
	public:
		constexpr static auto start_width = 1600;
		constexpr static auto start_height = 900;

		struct window_deleter
		{
			void operator()(GLFWwindow* window) const;
		};
		using window_ptr = std::unique_ptr<GLFWwindow, window_deleter>;

		default_app(std::string const& window_title);
		~default_app();

		bool begin_frame();
		int end_frame();

		std::chrono::duration<double> current_delta_time() const;
		window_ptr const& window() const;
		draw_state & default_draw_state() ;
		rnu::cameraf const& default_camera() const;
		texture_provider const& default_texture_provider() const;
		texture_provider & default_texture_provider() ;
		render_target const& default_render_target() const;
		render_target & default_render_target() ;

	private:
		void update_controls(std::chrono::duration<double> dt);

		struct
		{
			smooth<rnu::vec3> axis = { {0,0,0} };
			smooth<float> mx = { 0.f };
			smooth<float> my = { 0.f };
		} _camera_controls;

		struct
		{
			texture_provider::handle color;
			texture_provider::handle depth_stencil;
		} _draw_textures;

		std::chrono::steady_clock::time_point _last_frame_time;
		std::chrono::duration<double> _current_delta_time;
		window_ptr _window;
		draw_state _draw_state;
		rnu::cameraf _default_camera;
		texture_provider _texture_provider;
		render_target _default_render_target;
		bool _is_iconified = false;
		bool _override_controls = false;
		std::unordered_map<int, bool> _mouse_down;
	};

}