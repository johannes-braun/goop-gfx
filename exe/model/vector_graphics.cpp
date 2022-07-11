#include "vector_graphics.hpp"
#include <vectors/skyline_packer.hpp>
#include <execution>
#include <vectors/lines.hpp>
#include <stb_image_write.h>

namespace goop
{
  void vector_graphics_holder_base::load(int atlas_width, float sdf_width, std::span<goop::vector_image const> graphics)
  {
    float const scale = 2.0f; // todo
    _sdf_width = sdf_width;
    _width = atlas_width;
    _packed_bounds.reserve(graphics.size());
    _default_bounds.reserve(graphics.size());
    for (auto const& img : graphics)
    {
      auto b = img.bounds();
      b.position *= scale;
      b.size *= scale;
      b.position -= _sdf_width;
      b.size += 2 * _sdf_width;
      _packed_bounds.push_back(b);
      _default_bounds.push_back(b);
    }

    goop::skyline_packer skyline;
    _height = skyline.pack(_packed_bounds, _width);

    _image.resize(_width * _height);
    std::atomic_size_t index = 0;
    std::for_each(std::execution::par_unseq, begin(_packed_bounds), end(_packed_bounds), [this, &index, &graphics](auto const&) {
      auto const i = index++;
      auto& image = graphics[i];
      auto& bounds = _packed_bounds[i];

      auto const outline = goop::lines::to_line_segments(image);
      auto const at = [&](int x, int y) -> std::uint8_t& { return _image[x + y * _width]; };

      auto const min_x = (bounds.position.x);
      auto const min_y = (bounds.position.y);
      auto const max_x = (bounds.position.x + bounds.size.x);
      auto const max_y = (bounds.position.y + bounds.size.y);

      auto const voff = -bounds.position + image.bounds().position - rnu::vec2(_sdf_width, _sdf_width);

      for (int i = min_x; i <= max_x; ++i)
      {
        for (int j = min_y; j <= max_y; ++j)
        {
          auto const signed_distance = goop::lines::signed_distance(outline, { (i + voff.x) / scale, (max_y - j + voff.y) / scale }, 8) * scale;
          auto min = -_sdf_width;
          auto max = _sdf_width;

          auto const iix = 0 + i;
          auto const iiy = 0 + j;

          // Todo: letters should never overlap.
          at(iix, iiy) = std::uint8_t(
            (1 - std::clamp((signed_distance - min) / (max - min), 0.0f, 1.0f)) * 255
          );
        }
      }
      });
    auto const scale_by = 1.0 / rnu::vec2(_width, _height);
    for (auto& b : _packed_bounds)
    {
      b.position *= scale_by;
      b.size *= scale_by;
    }

    goop::texture result;
    result->allocate(goop::texture_type::t2d, goop::data_type::r8unorm, _width, _height, 1);
    result->set_data(0, 0, 0, _width, _height, 1, _image);
    _texture = std::move(result);
    stbi_write_jpg("../../../../../vector_atlas.jpg", _width, _height, 1, _image.data(), 95);
  }

  vector_graphics_holder_base::set_symbol_t vector_graphics_holder_base::get(std::size_t index) const
  {
    return { _default_bounds[index], _packed_bounds[index] };
  }

  goop::texture const& vector_graphics_holder_base::atlas_texture()
  {
    return _texture.value();
  }
  float vector_graphics_holder_base::sdf_width() const
  {
    return _sdf_width;
  }
}