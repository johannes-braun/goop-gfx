#pragma once

#include "geometry.hpp"

namespace goop
{
  struct primitive_data
  {
    std::vector<vertex> vertices;
    std::vector<unsigned> indices;
  };

  primitive_data make_cube();
  primitive_data make_sphere(int subdivisions = 3);

  std::vector<unsigned> to_line_sequence(std::vector<unsigned> const& triangle_indices);
}