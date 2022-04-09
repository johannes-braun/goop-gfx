#pragma once

#include <ranges>
#include <cstdint>
#include <span>
#include <vector>

struct attribute_type_info
{
  using uint_type = std::uint32_t;

  uint_type is_float : 1;
  uint_type is_normalized : 1;
  uint_type is_unsigned : 1;
  uint_type components : 2;
  uint_type bits0 : 6;
  uint_type bits1 : 6;
  uint_type bits2 : 6;
  uint_type bits3 : 6;
};

consteval auto encode_type(bool is_float, bool is_normalized, bool is_unsigned, size_t components, size_t bits0, size_t bits1 = 0, size_t bits2 = 0, size_t bits3 = 0)
{
  using uint_type = attribute_type_info::uint_type;
  uint_type result = 0;

  auto const p = [](auto val, auto off, auto mask) { return (static_cast<uint_type>(val) & mask) << off; };

  result |= p(is_float, 0, 0b1);
  result |= p(is_normalized, 1, 0b1);
  result |= p(is_unsigned, 2, 0b1);
  result |= p(components - 1, 3, 0b11);
  result |= p(bits0 - 1, 5, 0b111111);
  result |= p(bits1 - 1, 11, 0b111111);
  result |= p(bits2 - 1, 17, 0b111111);
  result |= p(bits3 - 1, 23, 0b111111);

  return result;
}

enum class attribute_type : std::uint32_t
{
#define gen_format_width(name, f, n, u, bits) \
  name##_x##bits = encode_type(f, n, u, 1, bits), \
  name##_x##bits##y##bits = encode_type(f, n, u, 2, bits, bits), \
  name##_x##bits##y##bits##z##bits##w##bits = encode_type(f, n, u, 4, bits, bits, bits, bits),

#define gen_formats_all_widths(name, f, n, u) \
  gen_format_width(name, f, n, u, 8) \
  gen_format_width(name, f, n, u, 16) \
  gen_format_width(name, f, n, u, 32) \
  gen_format_width(name, f, n, u, 64) 

  gen_formats_all_widths(unorm, false, true, true)
  gen_formats_all_widths(snorm, false, true, false)
  gen_formats_all_widths(uint, false, false, true)
  gen_formats_all_widths(sint, false, false, false)

  gen_format_width(float, true, false, false, 16)
  gen_format_width(float, true, false, false, 32)
  gen_format_width(float, true, false, false, 64)
};

constexpr attribute_type_info get_info(attribute_type type)
{
  using uint_type = attribute_type_info::uint_type;
  uint_type const as_uint = static_cast<uint_type>(type);

  auto const g = [&](auto off, auto mask) { return (as_uint >> off) & mask; };

  attribute_type_info info{};
  info.is_float = g(0, 0b1);
  info.is_normalized = g(1, 0b1);
  info.is_unsigned = g(2, 0b1);
  info.components = g(3, 0b11) + 1;
  info.bits0 = g(5, 0b111111) + 1;
  info.bits1 = g(11, 0b111111) + 1;
  info.bits2 = g(17, 0b111111) + 1;
  info.bits3 = g(23, 0b111111) + 1;
  return info;
}

class attribute_buffer
{
public:
  attribute_buffer(size_t stride)
    : _stride(stride)
  {

  }

  std::span<std::byte> allocate(size_t count)
  {
    size_t const begin = _data.size();
    _data.resize(_data.size() + count * _stride);
    return std::span(_data.begin() + begin, _data.end());
  }

  template<typename T>
  std::span<T const> unsafe_view_as() const
  {
    return std::span(reinterpret_cast<T const*>(_data.data()), _data.size() / _stride);
  }

  size_t num_elements() const {
    return _data.size() / _stride;
  }

private:
  size_t _stride;
  std::vector<std::byte> _data;
};

template<typename T, typename TMember>
constexpr size_t offset_of(TMember T::* member)
{
  T const* ptr = nullptr;
  return static_cast<size_t>(reinterpret_cast<intptr_t>(&(ptr->*member)));
}

class geometry_specs
{
public:
  struct attribute_info
  {
    attribute_type type;
    size_t size;
    ptrdiff_t offset;
  };

  void set_stride(size_t stride)
  {
    _stride = stride;
  }

  template<typename C, typename T>
  void add_attribute(attribute_type type, T C::* member)
  {
    add_attribute(type, sizeof(T), offset_of(member));
  }

  void add_attribute(attribute_type type, size_t size, ptrdiff_t offset)
  {
    _attributes.push_back({ type, size, offset });
  }

  size_t stride() const {
    return _stride;
  }

  std::vector<attribute_info> const& attributes() const {
    return _attributes;
  }

private:
  size_t _stride;
  std::vector<attribute_info> _attributes;
};

struct vertex_offset
{
  std::ptrdiff_t vertex_offset;
  std::size_t vertex_count;
  std::ptrdiff_t index_offset;
  std::size_t index_count;
};

class geometry
{
public:
  geometry(geometry_specs specs)
    : _specs(std::move(specs)), _attribute_buffer(_specs.stride())
  {

  }

  template<std::ranges::range T>
  vertex_offset append(T const& vertices, std::span<std::uint32_t> indices)
  {
    _dirty = true;
    auto dst = _attribute_buffer.allocate(std::size(vertices));

    vertex_offset result;
    result.vertex_offset = _attribute_buffer.num_elements();
    result.index_offset = _index_buffer.size();
    result.vertex_count = dst.size();
    result.index_count = indices.size();

    auto vertices_iter = vertices.begin();
    for (size_t i = 0; i < dst.size(); ++i)
    {
      for (auto const& attr : _specs.attributes())
        std::memcpy(dst.data() + attr.offset, reinterpret_cast<std::byte const*>(&*vertices_iter) + attr.offset, attr.size);

      dst = dst.subspan(_specs.stride());
      ++vertices_iter;
    }

    _index_buffer.insert(_index_buffer.end(), indices.begin(), indices.end());

    return result;
  }

private:
  bool _dirty = true;
  geometry_specs _specs;
  std::vector<std::uint32_t> _index_buffer;
  attribute_buffer _attribute_buffer;
};


void adskop()
{
  geometry_specs specs;
  specs.add_attribute(attribute_type::float_x32y32z32w32, &goop::vertex::position);

  geometry g({});
  std::vector<goop::vertex> verts;
  std::vector<std::uint32_t> ix;

  g.append(verts, ix);
}
