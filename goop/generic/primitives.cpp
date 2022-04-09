#include "primitives.hpp"
#include <array>
#include <numeric>

namespace goop
{
  constexpr std::array const cube_zpos{
     goop::vertex{.position = {-0.5, -0.5,  0.5}, .normal = { 0, 0, 1 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
     goop::vertex{.position = {-0.5,  0.5,  0.5}, .normal = { 0, 0, 1 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
     goop::vertex{.position = { 0.5, -0.5,  0.5}, .normal = { 0, 0, 1 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
     goop::vertex{.position = { 0.5,  0.5,  0.5}, .normal = { 0, 0, 1 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} },
  };
  constexpr std::array const cube_zneg{
      goop::vertex{.position = {-0.5, -0.5, -0.5}, .normal = { 0, 0, -1 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
      goop::vertex{.position = { 0.5, -0.5, -0.5}, .normal = { 0, 0, -1 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
      goop::vertex{.position = {-0.5,  0.5, -0.5}, .normal = { 0, 0, -1 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
      goop::vertex{.position = { 0.5,  0.5, -0.5}, .normal = { 0, 0, -1 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} }
  };
  constexpr std::array const cube_ypos{
      goop::vertex{.position = {-0.5, 0.5, -0.5}, .normal = { 0, 1, 0  }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
      goop::vertex{.position = { 0.5, 0.5, -0.5}, .normal = { 0, 1, 0  }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
      goop::vertex{.position = {-0.5, 0.5,  0.5}, .normal = { 0, 1, 0  }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
      goop::vertex{.position = { 0.5, 0.5,  0.5}, .normal = { 0, 1, 0  }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} },
  };
  constexpr std::array const cube_yneg{
      goop::vertex{.position = {-0.5, -0.5, -0.5}, .normal = { 0, -1, 0 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
      goop::vertex{.position = {-0.5, -0.5,  0.5}, .normal = { 0, -1, 0 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
      goop::vertex{.position = { 0.5, -0.5, -0.5}, .normal = { 0, -1, 0 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
      goop::vertex{.position = { 0.5, -0.5,  0.5}, .normal = { 0, -1, 0 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} }
  };
  constexpr std::array const cube_xpos{
      goop::vertex{.position = {0.5, -0.5, -0.5}, .normal = { 1, 0, 0 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
      goop::vertex{.position = {0.5, -0.5,  0.5}, .normal = { 1, 0, 0 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
      goop::vertex{.position = {0.5,  0.5, -0.5}, .normal = { 1, 0, 0 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
      goop::vertex{.position = {0.5,  0.5,  0.5}, .normal = { 1, 0, 0 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} },
  };
  constexpr std::array const cube_xneg{
      goop::vertex{.position = {-0.5, -0.5, -0.5}, .normal = { -1, 0, 0 }, .uv = {{0, 0}}, .color = {0, 0, 255, 255} },
      goop::vertex{.position = {-0.5,  0.5, -0.5}, .normal = { -1, 0, 0 }, .uv = {{1, 0}}, .color = {255, 0, 255, 255} },
      goop::vertex{.position = {-0.5, -0.5,  0.5}, .normal = { -1, 0, 0 }, .uv = {{0, 1}}, .color = {0, 255, 255, 255} },
      goop::vertex{.position = {-0.5,  0.5,  0.5}, .normal = { -1, 0, 0 }, .uv = {{1, 1}}, .color = {255, 255, 255, 255} }
  };
  constexpr std::array const cube_faces_indices{
    0u, 2u, 1u,
    1u, 2u, 3u
  };
  constexpr std::array const cube_faces{
    cube_zpos,
    cube_zneg,
    cube_ypos,
    cube_yneg,
    cube_xpos,
    cube_xneg,
  };

  primitive_data make_cube()
  {
    primitive_data data{};
    data.vertices.reserve(4 * 6);
    data.indices.reserve(6 * 6);

    for (auto const& f : cube_faces)
      for (auto const& v : f)
        data.vertices.push_back(v);

    for (int off = 0; off < 6; ++off)
      for (auto& i : cube_faces_indices)
        data.indices.push_back(i + off * 4);

    return data;
  }
  
  primitive_data make_sphere(int subdivisions)
  {
    primitive_data data{};

    auto const cube = make_cube();
    data.vertices.reserve(cube.indices.size());

    for (auto const& i : cube.indices) {
      data.vertices.push_back(cube.vertices[i]);
      data.vertices.back().position = normalize(data.vertices.back().position);
    }

    auto const sub = [](std::vector<goop::vertex>& v)
    {
      for (size_t offset = 0; offset < v.size(); offset += 12)
      {
        auto a = v[offset + 0];
        auto b = v[offset + 1];
        auto c = v[offset + 2];
        auto ab = mix(a, b, 0.5f);
        auto bc = mix(b, c, 0.5f);
        auto ca = mix(c, a, 0.5f);

        ab.position = normalize(ab.position);
        bc.position = normalize(bc.position);
        ca.position = normalize(ca.position);

        auto first = std::next(v.begin(), offset);
        first = v.erase(first, std::next(first, 3));
        v.insert(first, {
          a, ab, ca,
          ab, b, bc,
          ca, bc, c,
          ab, bc, ca
          });
      }
    };

    while (subdivisions--)
      sub(data.vertices);

    data.indices.resize(data.vertices.size());
    std::iota(begin(data.indices), end(data.indices), 0u);
    return data;
  }

  std::vector<unsigned> to_line_sequence(std::vector<unsigned> const& triangle_indices)
  {
    std::vector<unsigned> indices;
    indices.reserve(triangle_indices.size() * 2);
    for (unsigned i = 0; i < triangle_indices.size(); i += 3)
    {
      auto a = triangle_indices[i];
      auto b = triangle_indices[i+1];
      auto c = triangle_indices[i+2];
      indices.insert(indices.end(), { a, b, b, c, c, a });
    }
    return indices;
  }
}