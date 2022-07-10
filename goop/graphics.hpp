#pragma once

#define OPENGL 1

#if OPENGL
#include "opengl/texture.hpp"
#include "opengl/shader.hpp"
#include "opengl/sampler.hpp"
#include "opengl/mapped_buffer.hpp"
#include "opengl/draw_state.hpp"
#include "opengl/render_target.hpp"
#include "opengl/buffer.hpp"
#include "opengl/geometry_format.hpp"
#endif

#include "generic/texture_provider.hpp"
#include "generic/handle.hpp"

#include <string_view>
#include <string>
#include <memory>

namespace goop
{
#if OPENGL
  namespace graphics_impl = opengl;
#endif

  using texture = handle<graphics_impl::texture, texture_base>;
  using shader_pipeline = handle<graphics_impl::shader_pipeline, shader_pipeline_base>;
  using sampler = handle<graphics_impl::sampler, sampler_base>;
  using render_target = handle<graphics_impl::render_target, render_target_base>;
  using geometry_format = handle<graphics_impl::geometry_format, geometry_format_base>;
  using texture_provider = texture_provider_base<texture>;

  class shader : public handle<graphics_impl::shader, shader_base>
  {
  public:
    using base = handle<graphics_impl::shader, shader_base>;

    shader() = default;
    shader(shader_type type, std::string_view source, std::string* info_log = nullptr)
      : base() {
      (*this)->load_source(type, source, info_log);
    }
  };

  class draw_state : public handle<graphics_impl::draw_state, draw_state_base>
  {
  public:
    using base = handle<graphics_impl::draw_state, draw_state_base>;

    draw_state() = default;
    draw_state(GLFWwindow* window) : base()
    {
      (*this)->from_window(window);
    }
  };

  template<typename T>
  class mapped_buffer : public handle<graphics_impl::mapped_buffer<T>, mapped_buffer_base<T>>
  {
  public:
    using base = handle<graphics_impl::mapped_buffer<T>, mapped_buffer_base<T>>;

    mapped_buffer() = default;
    mapped_buffer(size_t elements) : base() { (*this)->allocate(elements); }
  };

  class buffer : public handle<graphics_impl::buffer, buffer_base>
  {
  public:
    buffer() = default;

    template<typename T>
    buffer(std::span<T> data)
    {
      (*this)->load(data);
    }

    template<typename T>
    buffer(std::initializer_list<T> data)
    {
      (*this)->load(std::span(data));
    }
  };
}