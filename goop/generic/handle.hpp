#pragma once

#include <memory>
#include <utility>

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

  template<typename T, typename Base = T>
  class handle : public handle_base<std::shared_ptr<T>, Base>
  {
  public:
    using base = handle<T, Base>;

    template<typename... Ts> requires std::constructible_from<T, Ts...>
    handle(Ts&&... t) : handle_base<std::shared_ptr<T>, Base>{ std::make_shared<T>(std::forward<Ts>(t)...) } {}
  };

  template<typename T>
  class handle_ref
  {
  public:
    template<typename Impl>
    handle_ref(handle<Impl, T> const& handle)
      : _handle(std::static_pointer_cast<T>(handle.native_ptr()))
    { }

    template<typename Impl, typename K>
    handle_ref(handle<Impl, K> const& handle) requires std::derived_from<K, T>
      : _handle(std::static_pointer_cast<T>(handle.native_ptr()))
    { }

    T& get();
    T const& get() const;

    T* operator->();
    T const* operator->() const;
    
    operator std::shared_ptr<T>& () {
      return _handle;
    }
    operator std::shared_ptr<T> const& () const {
      return _handle;
    }

    template<typename R>
    R* as() {
      return static_cast<R*>(_handle.get());
    }

    template<typename R>
    R const* as() const {
      return static_cast<R*>(_handle.get());
    }

  private:
    std::shared_ptr<T> _handle;
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

  template<typename T>
  T& handle_ref<T>::get()
  {
    return *_handle;
  }
  template<typename T>
  T const& handle_ref<T>::get() const
  {
    return *_handle;
  }

  template<typename T>
  T* handle_ref<T>::operator->()
  {
    return _handle.get();
  }
  template<typename T>
  T const* handle_ref<T>::operator->() const
  {
    return _handle.get();
  }
}