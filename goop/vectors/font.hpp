#pragma once

#include <filesystem>
#include <unordered_map>
#include <memory>
#include "lines.hpp"
#include <fstream>

namespace goop
{
  enum class glyph_id : std::uint32_t {};

  class glyph_map_base
  {
  public:
    virtual std::optional<glyph_id> glyph_index(char32_t character) const = 0;
  };

  struct rect
  {
    rnu::vec2 min;
    rnu::vec2 max;
  };

  class font_file
  {
  public:
    font_file(std::filesystem::path const& path);

    std::optional<glyph_id> glyph_index(char32_t character) const;

    std::vector<lines::line_segment> outline(glyph_id glyph, rect* bounds = nullptr) const;

    float units_per_em() const;

  private:
    std::size_t glyph_offset_bytes(glyph_id glyph) const;

    struct table
    {
      std::uint32_t tag;
      std::uint32_t check_sum;
      std::uint32_t offset;
      std::uint32_t length;
    };

    struct header
    {
      std::uint16_t version_major;
      std::uint16_t version_minor;
      std::uint16_t revision_major;
      std::uint16_t revision_minor;
      std::uint32_t checksum_adjustment;
      std::uint32_t magic_number;
      std::uint16_t flags;
      std::uint16_t units_per_em;
      std::int64_t created;
      std::int64_t modified;
      std::int16_t x_min;
      std::int16_t y_min;
      std::int16_t x_max;
      std::int16_t y_max;
      std::uint16_t mac_style;
      std::uint16_t lowest_recommended_ppem;
      std::int16_t font_direction_hint;
      std::int16_t index_to_loc_format;
      std::int16_t glyph_data_format;
    };

    struct maxp
    {
      std::uint16_t version_major;
      std::uint16_t version_minor;
      std::uint16_t num_glyphs;
      std::uint16_t max_points;
      std::uint16_t max_contours;
      std::uint16_t max_component_points;
      std::uint16_t max_component_contours;
      std::uint16_t max_zones;
      std::uint16_t max_twilight_points;
      std::uint16_t max_storage;
      std::uint16_t max_function_defs;
      std::uint16_t max_stack_elements;
      std::uint16_t max_size_of_instructions;
      std::uint16_t max_component_elements;
      std::uint16_t max_component_depth;
    };

    mutable std::ifstream _file;
    header _header;
    maxp _maxp;
    std::unordered_map<std::uint32_t, table> _tables;
    std::unique_ptr<glyph_map_base> _glyph_map;
    std::vector<std::uint32_t> _glyph_offsets;
  };
}