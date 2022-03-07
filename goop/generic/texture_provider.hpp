#pragma once

#include "texture.hpp"
#include "../hash.hpp"
#include <unordered_map>

namespace goop
{
  template<typename TextureHandle>
  class texture_provider_base
  {
  public:
    using handle = TextureHandle;
    using pointer = typename handle::pointer_type;

    handle acquire(texture_type type, data_type data, int w, int h, int d, int mipmap_levels_or_samples);
    handle acquire(texture_type type, data_type data, int w, int h, int mipmap_levels_or_samples);
    handle acquire(texture_type type, data_type data, int w, int mipmap_levels_or_samples);
    void free(handle const& handle);

  private:
    std::pair<bool, handle> acquire_handle(texture_type type, data_type data, int w, int h, int d, int mipmap_levels_or_samples);
    std::unordered_map<pointer, size_t> _hashes;
    std::unordered_multimap<std::size_t, handle> _acquired_handles;
    std::unordered_multimap<std::size_t, handle> _free_handles;
  };

  template<typename TextureHandle>
  typename texture_provider_base<TextureHandle>::handle texture_provider_base<TextureHandle>::acquire(texture_type type, data_type data, int w, int h, int d, int mipmap_levels_or_samples)
  {
    auto [is_new, hnd] = acquire_handle(type, data, w, h, d, mipmap_levels_or_samples);
    if (is_new)
      hnd->allocate(type, data, w, h, d, mipmap_levels_or_samples);
    return hnd;
  }

  template<typename TextureHandle>
  typename texture_provider_base<TextureHandle>::handle texture_provider_base<TextureHandle>::acquire(texture_type type, data_type data, int w, int h, int mipmap_levels_or_samples)
  {
    auto [is_new, hnd] = acquire_handle(type, data, w, h, 1, mipmap_levels_or_samples);
    if (is_new)
      hnd->allocate(type, data, w, h, mipmap_levels_or_samples);
    return hnd;
  }

  template<typename TextureHandle>
  typename texture_provider_base<TextureHandle>::handle texture_provider_base<TextureHandle>::acquire(texture_type type, data_type data, int w, int mipmap_levels_or_samples)
  {
    auto [is_new, hnd] = acquire_handle(type, data, w, 1, 1, mipmap_levels_or_samples);
    if (is_new)
      hnd->allocate(type, data, w, mipmap_levels_or_samples);
    return hnd;
  }

  template<typename TextureHandle>
  void texture_provider_base<TextureHandle>::free(handle const& handle)
  {
    auto const h = _hashes[handle.native_ptr()];
    auto [begin, end] = _acquired_handles.equal_range(h);
    for (; begin != end; ++begin)
    {
      if (begin->second.native_ptr() == handle.native_ptr())
      {
        _acquired_handles.erase(begin);
        _free_handles.emplace(h, handle);
        return;
      }
    }
  }

  template<typename TextureHandle>
  std::pair<bool, typename texture_provider_base<TextureHandle>::handle> texture_provider_base<TextureHandle>::acquire_handle(texture_type type, data_type data, int w, int h, int d, int mipmap_levels_or_samples)
  {
    auto const next_hash = hash(type, data, w, h, d, mipmap_levels_or_samples);

    auto const found_iter = _free_handles.find(next_hash);

    if (found_iter != _free_handles.end())
    {
      auto [cur_hash, cur_handle] = *found_iter;
      _free_handles.erase(found_iter);
      return { false, _acquired_handles.emplace(cur_hash, std::move(cur_handle))->second };
    }
    else
    {
      handle new_handle{};
      _hashes[new_handle.native_ptr()] = next_hash;
      return { true, _acquired_handles.emplace(next_hash, std::move(new_handle))->second };
    }
  }
}