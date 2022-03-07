#pragma once

#include <array>
#include <optional>
#include <type_traits>
#include <cstdint>

namespace goop
{
  static constexpr std::size_t runtime_exponent = 0;

  namespace detail
  {
    constexpr std::size_t exp2(std::size_t val)
    {
      if (val == 0)
        return 1;
      return 2 * exp2(val - 1);
    }

    template<typename Data, std::size_t Exp>
    struct octree_data_holder
    {
      struct node
      {
        std::optional<Data> value;
      };

      static constexpr auto exponent = Exp;
      static constexpr auto dimension = detail::exp2(exponent);
      static constexpr auto num_nodes = exponent * dimension * dimension * dimension;
      static constexpr auto required_memory = sizeof(node) * num_nodes;

      std::array<node, num_nodes> _nodes;
    };

    template<typename Data>
    struct octree_data_holder<Data, runtime_exponent>
    {
      struct node
      {
        std::optional<Data> value;
      };

      constexpr octree_data_holder<Data, runtime_exponent>(std::size_t exponent);

      std::size_t exponent;
      std::size_t dimension;
      std::size_t num_nodes;
      std::size_t required_memory;
      std::vector<node> _nodes;
    };
  }

  template<typename T>
  concept octree_value_type = std::is_fundamental_v<T>;

  template<octree_value_type Data, std::size_t Exp = runtime_exponent>
  class dynamic_octree : protected detail::octree_data_holder<Data, Exp>
  {

  public:
    using value_type = Data;
    using node = typename detail::octree_data_holder<Data, Exp>::node;

    struct dimension_value
    {
      std::size_t base_x;
      std::size_t base_y;
      std::size_t base_z;
      std::size_t size;
      Data value;
    };

    constexpr dynamic_octree() requires(Exp != runtime_exponent);
    constexpr dynamic_octree(std::size_t exponent) requires(Exp == runtime_exponent);

    constexpr dimension_value info(int x, int y, int z) const;
    constexpr Data at(int x, int y, int z) const;
    constexpr void emplace(int x, int y, int z, Data t);

    constexpr std::size_t border_size() const;

  private:
    constexpr std::ptrdiff_t children_start_index(std::ptrdiff_t index) const;
    constexpr std::ptrdiff_t parent_index(std::ptrdiff_t index) const;
    constexpr std::ptrdiff_t next(int x, int y, int z, int half_dim, std::ptrdiff_t src) const;

    constexpr std::span<node const> child_nodes(std::ptrdiff_t index) const;

    constexpr void merge(std::ptrdiff_t index, std::optional<Data> with_value);
    constexpr void divide(std::ptrdiff_t index);

    using detail::octree_data_holder<Data, Exp>::_nodes;
    using detail::octree_data_holder<Data, Exp>::dimension;
  };
}

#include "dynamic_octree.inl.hpp"