#include "dynamic_octree.hpp"
namespace goop
{
  template<typename Data>
  constexpr detail::octree_data_holder<Data, runtime_exponent>::octree_data_holder(std::size_t exponent)
  {
    this->exponent = exponent;
    this->dimension = detail::exp2(exponent);
    this->num_nodes = exponent * dimension * dimension * dimension;
    this->required_memory = sizeof(node) * num_nodes;
    _nodes.resize(this->num_nodes);
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr dynamic_octree<Data, Exp>::dynamic_octree() requires(Exp != runtime_exponent)
  {
    _nodes[0].value = Data{};
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr dynamic_octree<Data, Exp>::dynamic_octree(std::size_t exponent) requires(Exp == runtime_exponent)
    : detail::octree_data_holder<Data, runtime_exponent>::octree_data_holder(exponent)
  {
    _nodes[0].value = Data{};
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr typename dynamic_octree<Data, Exp>::dimension_value dynamic_octree<Data, Exp>::info(int x, int y, int z) const
  {
    std::ptrdiff_t node_index = 0;
    int half_dim = dimension;

    int pos_x = 0;
    int pos_y = 0;
    int pos_z = 0;
    int depth = 0;

    while (!_nodes[node_index].value)
    {
      auto const next_index = next(x, y, z, half_dim /= 2, node_index);
      if (next_index >= _nodes.size())
        break;
      node_index = next_index;

      x %= half_dim;
      y %= half_dim;
      z %= half_dim;

      pos_x += (x >= half_dim) ? half_dim : 0;
      pos_y += (y >= half_dim) ? half_dim : 0;
      pos_z += (z >= half_dim) ? half_dim : 0;
      ++depth;
    }

    return dimension_value{
      .base_x = static_cast<std::size_t>(pos_x),
      .base_y = static_cast<std::size_t>(pos_y),
      .base_z = static_cast<std::size_t>(pos_z),
      .size = dimension / static_cast<int>(std::exp2(depth)),
      .value = *_nodes[node_index].value
    };
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr Data dynamic_octree<Data, Exp>::at(int x, int y, int z) const
  {
    if (x >= dimension || x < 0 || y >= dimension || y < 0 || z >= dimension || z < 0)
      return Data{ };

    return info(x, y, z).value;
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr void dynamic_octree<Data, Exp>::emplace(int x, int y, int z, Data t)
  {
    if (x >= dimension || x < 0 || y >= dimension || y < 0 || z >= dimension || z < 0)
      return;

    std::ptrdiff_t node_index = 0;
    int half_dim = dimension;

    while (node_index < _nodes.size())
    {
      while (!_nodes[node_index].value)
      {
        auto const next_index = next(x, y, z, half_dim /= 2, node_index);
        if (next_index >= _nodes.size())
          break;
        node_index = next_index;
        x %= half_dim;
        y %= half_dim;
        z %= half_dim;
      }
      if (_nodes[node_index].value == t)
        return;
      else if (!_nodes[node_index].value || children_start_index(node_index) >= _nodes.size())
      {
        _nodes[node_index].value = t;
        auto parent = parent_index(node_index);
        auto children = child_nodes(parent);
        while (std::all_of(begin(children), end(children), [&](node const& node) { return node.value == t; }))
        {
          merge(parent, t);
          parent = parent_index(parent);
          children = child_nodes(parent);
        }
        return;
      }
      else
      {
        divide(node_index);
      }
    }
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr std::size_t dynamic_octree<Data, Exp>::border_size() const
  {
    return dimension;
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr std::ptrdiff_t dynamic_octree<Data, Exp>::children_start_index(std::ptrdiff_t index) const
  {
    return 8 + (index * 8);
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr std::ptrdiff_t dynamic_octree<Data, Exp>::parent_index(std::ptrdiff_t index) const
  {
    return (index - 8) / 8;
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr std::ptrdiff_t dynamic_octree<Data, Exp>::next(int x, int y, int z, int half_dim, std::ptrdiff_t src) const
  {
    int const index = (x >= half_dim) | ((y >= half_dim) << 1) | ((z >= half_dim) << 2);
    return children_start_index(src) + index;
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr std::span<typename dynamic_octree<Data, Exp>::node const> dynamic_octree<Data, Exp>::child_nodes(std::ptrdiff_t index) const
  {
    return { &_nodes[children_start_index(index)], 8 };
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr void dynamic_octree<Data, Exp>::merge(std::ptrdiff_t index, std::optional<Data> with_value)
  {
    if (index >= _nodes.size())
      return;

    if (!_nodes[index].value)
    {
      auto const children_index = children_start_index(index);
      for (std::ptrdiff_t i = 0; i < 8; ++i)
      {
        auto const child = i + children_index;

        merge(child, std::nullopt);
      }
    }

    _nodes[index].value = with_value;
  }

  template<octree_value_type Data, std::size_t Exp>
  constexpr void dynamic_octree<Data, Exp>::divide(std::ptrdiff_t index)
  {
    if (!_nodes[index].value || children_start_index(index) >= _nodes.size())
      return;

    auto const children_index = children_start_index(index);
    for (std::ptrdiff_t i = 0; i < 8; ++i)
    {
      auto const child = i + children_index;
      _nodes[child].value = _nodes[index].value;
    }

    _nodes[index].value = std::nullopt;
  }
}