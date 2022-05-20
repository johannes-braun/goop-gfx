#include "skyline_packer.hpp"
#include <numeric>
#include <ranges>

namespace goop
{
  int skyline_packer::pack(std::span<rnu::rect2f> rectangles, int width)
  {
    std::vector<std::size_t> indices(rectangles.size());
    std::iota(begin(indices), end(indices), 0ull);

    std::ranges::sort(indices, [&](std::size_t a, std::size_t b) {  return rectangles[a].size.y > rectangles[b].size.y; });

    for (auto& r : rectangles)
    {
      r.size.x = std::ceilf(r.size.x);
      r.size.y = std::ceilf(r.size.y);
      r.position.x = std::floorf(r.position.x);
      r.position.y = std::floorf(r.position.y);
    }

    _nodes.push_back(node{ 0, 0, (width) });

    int result_height = 0;
    for (auto index : indices)
    {
      rnu::rect2f& r = rectangles[index];

      auto best_iter = _nodes.end();
      int best_num_nodes = 0;
      int best_width = std::numeric_limits<int>::max();
      int best_y = std::numeric_limits<int>::max();

      for (auto iter = _nodes.begin(); iter != _nodes.end(); ++iter)
      {
        if (iter->y > best_y)
          continue;

        int full_width = iter->width;
        auto next = std::next(iter);
        int y = iter->y;
        int nodes = 1;

        while (full_width <= r.size.x && next != _nodes.end())
        {
          full_width += next->width;
          y = std::max(next->y, y);
          nodes++;
          next = std::next(next);
        }

        if (full_width < r.size.x)
          break;

        if (best_y > y)
        {
          best_y = y;
          best_width = full_width;
          best_iter = iter;
          best_num_nodes = nodes;
        }
      }

      r.position.x = best_iter->x;
      r.position.y = best_y;

      result_height = std::max<int>(result_height, r.position.y + r.size.y);

      node new_node;
      new_node.x = best_iter->x;
      new_node.y = best_y + r.size.y;
      new_node.width = r.size.x;

      auto const last_elem = std::next(best_iter, best_num_nodes - 1);

      node new_next_node;
      new_next_node.x = best_iter->x + r.size.x;
      new_next_node.y = last_elem->y;
      new_next_node.width = best_width - new_node.width;

      auto const pos = _nodes.erase(best_iter, std::next(last_elem));
      if (new_next_node.width != 0 && new_node.width != 0)
        _nodes.insert(pos, { new_node, new_next_node });
      else if (new_next_node.width == 0 && new_node.width != 0)
        _nodes.insert(pos, new_node);
      else if (new_next_node.width != 0 && new_node.width == 0)
        _nodes.insert(pos, new_next_node);
    }

    return result_height + 1;
  }
}