#pragma once

#include "handle.hpp"
#include "buffer.hpp"
#include "draw_state.hpp"
#include <rnu/math/math.hpp>
#include <algorithm/utility.hpp>
#include <optional>

namespace goop
{
  struct attribute_format
  {
    enum class data_type
    {
      f,
      i,
      u,
    };
    enum class component_count
    {
      scalar,
      vec2,
      vec3,
      vec4
    };
    enum class bit_width
    {
      x8,
      x16,
      x32,
      x64
    };

    data_type type;
    component_count components;
    bit_width bits;
    bool normalized;
  };

  namespace detail
  {
    template<typename T, bool N>
    struct default_attribute_t;

#define GOOP_gen_def_attr(T, Dt, C, B, N) \
  template<> struct default_attribute_t<T, N>\
  {\
    constexpr static attribute_format value{\
      .type = attribute_format::data_type:: Dt,\
      .components = attribute_format::component_count:: C, \
      .bits = attribute_format::bit_width:: B, \
      .normalized = N \
    };\
  };
#define GOOP_COMMA ,
#define GOOP_gen_all_attrs(T, Dt, B, N) \
  GOOP_gen_def_attr(T, Dt, scalar, B, N) \
  GOOP_gen_def_attr(rnu::vec<T GOOP_COMMA 2>, Dt, vec2, B, N) \
  GOOP_gen_def_attr(rnu::vec<T GOOP_COMMA 3>, Dt, vec3, B, N) \
  GOOP_gen_def_attr(rnu::vec<T GOOP_COMMA 4>, Dt, vec4, B, N) \

    GOOP_gen_all_attrs(float, f, x32, false);
    GOOP_gen_all_attrs(double, f, x64, false);
    GOOP_gen_all_attrs(std::int8_t, i, x8, false);
    GOOP_gen_all_attrs(std::int16_t, i, x16, false);
    GOOP_gen_all_attrs(std::int32_t, i, x32, false);
    GOOP_gen_all_attrs(std::int64_t, i, x64, false);
    GOOP_gen_all_attrs(std::uint8_t, u, x8, false);
    GOOP_gen_all_attrs(std::uint16_t, u, x16, false);
    GOOP_gen_all_attrs(std::uint32_t, u, x32, false);
    GOOP_gen_all_attrs(std::uint64_t, u, x64, false);
    GOOP_gen_all_attrs(std::int8_t, i, x8, true);
    GOOP_gen_all_attrs(std::int16_t, i, x16, true);
    GOOP_gen_all_attrs(std::int32_t, i, x32, true);
    GOOP_gen_all_attrs(std::int64_t, i, x64, true);
    GOOP_gen_all_attrs(std::uint8_t, u, x8, true);
    GOOP_gen_all_attrs(std::uint16_t, u, x16, true);
    GOOP_gen_all_attrs(std::uint32_t, u, x32, true);
    GOOP_gen_all_attrs(std::uint64_t, u, x64, true);
#undef GOOP_gen_all_attrs
#undef GOOP_gen_def_attr
#undef GOOP_COMMA
  }

  template<typename T, bool Normalized>
  static constexpr auto default_format_for = detail::default_attribute_t<T, Normalized>::value;

  enum class attribute_repetition
  {
    per_vertex,
    per_instance
  };

  struct attribute
  {
    attribute_format format;
    std::size_t binding;
    std::ptrdiff_t structure_offset;
  };

  template<typename T, bool N>
  static constexpr attribute attribute_for(std::size_t binding, std::uintptr_t offset)
  {
    return attribute{
      .format = default_format_for<T, N>,
      .binding = binding,
      .structure_offset = static_cast<std::ptrdiff_t>(offset)
    };
  }

  template<typename T, bool N, typename MT, typename MC>
  static constexpr attribute attribute_for(std::size_t binding, MT MC::* member)
  {
    return attribute_for<T, N>(binding, offset_of(member));
  }

  template<bool N, typename MT, typename MC>
  static constexpr attribute attribute_for(std::size_t binding, MT MC::* member)
  {
    return attribute_for<MT, N>(binding, offset_of(member));
  }

  template<typename T, bool N, typename MT, typename MC, size_t K>
  static constexpr attribute attribute_for(std::size_t binding, MT (MC::* member)[K], size_t index)
  {
    return attribute_for<T, N>(binding, offset_of(member, index));
  }

  template<bool N, typename MT, typename MC, size_t K>
  static constexpr attribute attribute_for(std::size_t binding, MT (MC::* member)[K], size_t index)
  {
    return attribute_for<MT, N>(binding, offset_of(member, index));
  }

  enum class primitive_type
  {
    points,
    lines,
    line_strip,
    triangles,
    triangle_strip
  };

  struct draw_info_indexed
  {
    std::uint32_t count;
    std::uint32_t instance_count;
    std::uint32_t first_index;
    std::uint32_t base_vertex;
    std::uint32_t base_instance;
  };

  struct draw_info_array
  {
    std::uint32_t count;
    std::uint32_t instance_count;
    std::uint32_t first_index;
    std::uint32_t base_instance;
  };

  class geometry_format_base
  {
  public:
    virtual void set_attribute(std::size_t index, std::optional<attribute> attr) = 0;
    virtual void set_binding(std::size_t binding, std::size_t stride, attribute_repetition repeat = attribute_repetition::per_vertex) = 0;
    virtual void use_index_buffer(draw_state_base& state, attribute_format::bit_width bits, handle_ref<buffer_base> buffer) = 0;
    virtual void use_buffer(draw_state_base& state, std::size_t binding, handle_ref<buffer_base> buffer, std::ptrdiff_t offset = 0) = 0;
    virtual void draw_indexed(draw_state_base& state, primitive_type type, draw_info_indexed const& info) = 0;
    virtual void draw_indexed(draw_state_base& state, primitive_type type, buffer_base const& indirect_buffer, std::size_t count, std::ptrdiff_t offset = 0) = 0;
    virtual void draw_array(draw_state_base& state, primitive_type type, draw_info_array const& info) = 0;
    virtual void draw_array(draw_state_base& state, primitive_type type, buffer_base const& indirect_buffer, std::size_t count, std::ptrdiff_t offset = 0) = 0;
  };
}