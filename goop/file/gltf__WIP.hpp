#pragma once

#include <vector>
#include <animation/animation.hpp>
#include <graphics.hpp>
#include <filesystem>

namespace goop
{
  struct material
  {
    texture diffuse_map;
    struct
    {
      rnu::vec4 color;
      uint32_t has_texture;
    } data;

    mapped_buffer<decltype(data)> data_buffer{ 1 };
    bool dirty = true;

    void bind(draw_state& state) {
      if (dirty)
      {
        dirty = false;
        data_buffer->write(data);
      }
      diffuse_map->bind(state, 0);
      data_buffer->bind(state, 2);
    }
  };

  struct gltf_data
  {
    transform_tree nodes;
    std::vector<material> materials;
    std::vector<skin> skins;
    std::vector<std::vector<vertex_offset>> material_geometries;
    std::unordered_map<std::string, joint_animation> animations;
  };

  gltf_data load_gltf(std::filesystem::path const& path, texture_provider& tex_provider, geometry& main_geometry);
}