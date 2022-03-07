#pragma once

#include "generic/texture_provider.hpp"
#include "opengl/texture.hpp"
#include "opengl/shader.hpp"
#include "opengl/sampler.hpp"
#include "opengl/geometry.hpp"
#include "opengl/mapped_buffer.hpp"
#include "opengl/draw_state.hpp"
#include "opengl/render_target.hpp"

#include <string_view>
#include <string>
#include <memory>

namespace goop
{
  template<typename Ptr, typename Val>
  class handle_base
  {
  public:
    using pointer_type = Ptr;

    handle_base(Ptr&& ptr) : _impl{ std::move(ptr) } {}
    ~handle_base() = default;

    operator Val& ();
    operator Val const& () const;
    void reset(Ptr&& newPtr = nullptr);
    Val* operator->();
    Val const* operator->() const;
    Val& operator*();
    Val const& operator*() const;

    Ptr const& native_ptr() const;

  private:
    Ptr _impl;
  };

  template<typename T, typename Base>
  class handle : public handle_base<std::shared_ptr<T>, Base>
  {
  public:
    using base = handle<T, Base>;

    handle() : handle_base<std::shared_ptr<T>, Base>{ std::make_shared<T>() } {}
  };

#if 1
  namespace graphics_impl = opengl;
#endif

  using texture = handle<graphics_impl::texture, texture_base>;
  using shader_pipeline = handle<graphics_impl::shader_pipeline, shader_pipeline_base>;
  using sampler = handle<graphics_impl::sampler, sampler_base>;
  using geometry = handle<graphics_impl::geometry, geometry_base>;
  using render_target = handle<graphics_impl::render_target, render_target_base>;
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
  
   
  template<typename Ptr, typename Val>
  inline handle_base<Ptr, Val>::operator Val& ()
  {
    return *_impl;
  }

  template<typename Ptr, typename Val>
  inline handle_base<Ptr, Val>::operator Val const& () const
  {
    return *_impl;
  }

  template<typename Ptr, typename Val>
  inline void handle_base<Ptr, Val>::reset(Ptr&& newPtr)
  {
    _impl = std::move(newPtr);
  }

  template<typename Ptr, typename Val>
  inline Val* handle_base<Ptr, Val>::operator->()
  {
    return &*_impl;
  }

  template<typename Ptr, typename Val>
  inline Val const* handle_base<Ptr, Val>::operator->() const
  {
    return &*_impl;
  }

  template<typename Ptr, typename Val>
  inline Val& handle_base<Ptr, Val>::operator*()
  {
    return *_impl;
  }

  template<typename Ptr, typename Val>
  inline Val const& handle_base<Ptr, Val>::operator*() const
  {
    return *_impl;
  }

  template<typename Ptr, typename Val>
  inline Ptr const& handle_base<Ptr, Val>::native_ptr() const
  {
    return _impl;
  }
}