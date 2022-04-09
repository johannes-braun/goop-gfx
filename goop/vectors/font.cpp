#include "font.hpp"
#include <span>
#include <ranges>
#include <array>

namespace goop
{
  class glyph_map_format0 : public glyph_map_base
  {
  public:
    std::array<std::uint8_t, 256> indices;

    virtual std::optional<glyph_id> glyph_index(char32_t character) const override
    {
      if (character < 0 || character >= 256)
        return std::nullopt;
      return glyph_id{ indices[character] };
    }
  };

  class glyph_map_format4 : public glyph_map_base
  {
  public:
    std::uint16_t seg_count_x2;
    std::uint16_t search_range;
    std::uint16_t entry_selector;
    std::uint16_t range_shift;

    struct segment_list
    {
      std::vector<std::uint16_t> segment_data;
      std::span<std::uint16_t> end_codes;
      std::span<std::uint16_t> start_codes;
      std::span<std::uint16_t> id_delta;
      std::span<std::uint16_t> id_range_offset_extended;
    } segments;

    virtual std::optional<glyph_id> glyph_index(char32_t character) const override
    {
      auto const iter = std::ranges::lower_bound(segments.end_codes, std::uint16_t(character));
      auto const index = std::distance(segments.end_codes.begin(), iter);
      auto const start_code = segments.start_codes[index];
      if (start_code > character)
        return std::nullopt;

      auto const id_range_offset = segments.id_range_offset_extended[index];

      if (id_range_offset == 0)
        return glyph_id{ (segments.id_delta[index] + character) % 65536 };

      return glyph_id{ segments.id_range_offset_extended[
        index + id_range_offset / sizeof(uint16_t) + (character - start_code)
      ] };
    }
  };


  std::uint8_t read_u8(std::istream& stream)
  {
    std::uint8_t val{};
    stream.read(reinterpret_cast<char*>(&val), sizeof(val));
    return val;
  }

  std::uint16_t read_u16(std::istream& stream)
  {
    return (read_u8(stream) << 8) | read_u8(stream);
  }

  std::uint32_t read_u32(std::istream& stream)
  {
    return (read_u16(stream) << 16) | read_u16(stream);
  }

  std::uint64_t read_u64(std::istream& stream)
  {
    return (std::uint64_t(read_u32(stream)) << 32) | read_u32(stream);
  }

  consteval std::uint32_t make_tag(char const (&arr)[5])
  {
    return arr[3] | (arr[2] << 8) | (arr[1] << 16) | (arr[0] << 24);
  }

  std::optional<glyph_id> font_file::glyph_index(char32_t character) const
  {
    return _glyph_map->glyph_index(character);
  }

  std::size_t font_file::glyph_offset_bytes(glyph_id glyph) const
  {
    return _glyph_offsets[size_t(glyph)];
  }

  font_file::font_file(std::filesystem::path const& path)
    : _file(path, std::ios::binary)
  {
    struct offset_subtable
    {
      uint32_t scaler;
      uint16_t num_tables;
      uint16_t search_ranges;
      uint16_t entry_selector;
      uint16_t range_shift;
    } offsets;

    // read offset subtable
    {
      offsets.scaler = read_u32(_file);
      offsets.num_tables = read_u16(_file);
      offsets.search_ranges = read_u16(_file);
      offsets.entry_selector = read_u16(_file);
      offsets.range_shift = read_u16(_file);
    }

    for (uint16_t i = 0; i < offsets.num_tables; ++i)
    {
      std::uint32_t const tag = read_u32(_file);
      _tables.emplace(tag, table{
        .tag = tag,
        .check_sum = read_u32(_file),
        .offset = read_u32(_file),
        .length = read_u32(_file)
      });
    }

    // read header
    {
      constexpr static auto const tag = make_tag("head");
      _file.seekg(_tables[tag].offset);
      _header.version_major = read_u16(_file);
      _header.version_minor = read_u16(_file);
      _header.revision_major = read_u16(_file);
      _header.revision_minor = read_u16(_file);
      _header.checksum_adjustment = read_u32(_file);
      _header.magic_number = read_u32(_file);

      if (_header.magic_number != 0x5F0F3CF5)
        throw std::runtime_error("Error while reading font: invalid magic number.");

      _header.flags = read_u16(_file);
      _header.units_per_em = read_u16(_file);
      _header.created = read_u64(_file);
      _header.modified = read_u64(_file);
      _header.x_min = read_u16(_file);
      _header.y_min = read_u16(_file);
      _header.x_max = read_u16(_file);
      _header.y_max = read_u16(_file);
      _header.mac_style = read_u16(_file);
      _header.lowest_recommended_ppem = read_u16(_file);
      _header.font_direction_hint = read_u16(_file);
      _header.index_to_loc_format = read_u16(_file);
      _header.glyph_data_format = read_u16(_file);
    }

    // read cmap
    {
      constexpr static auto const tag = make_tag("cmap");
      auto const offset = _tables[tag].offset;
      _file.seekg(offset);
      std::uint16_t version = read_u16(_file);
      std::uint16_t num_subtables = read_u16(_file);

      struct encoding_record
      {
        std::uint16_t platform_id;
        std::uint16_t encoding_id;
        std::uint32_t subtable_offset;
      };

      std::vector<encoding_record> encodings(num_subtables);
      for (auto& enc : encodings)
      {
        enc.platform_id = read_u16(_file);
        enc.encoding_id = read_u16(_file);
        enc.subtable_offset = read_u32(_file);
      }
      
      // go to subtable
      _file.seekg(offset + encodings[0].subtable_offset);
      std::uint16_t const format = read_u16(_file);
      std::uint16_t const length = read_u16(_file);
      std::uint16_t const language = read_u16(_file);

      if (format == 0)
      {
        auto map = std::make_unique<glyph_map_format0>();
        for (int i = 0; i < 256; ++i)
          map->indices[i] = read_u8(_file);
        _glyph_map = std::move(map);
      }
      else if (format == 4)
      {
        auto map = std::make_unique<glyph_map_format4>();
        map->seg_count_x2 = read_u16(_file);
        map->search_range = read_u16(_file);
        map->entry_selector = read_u16(_file);
        map->range_shift = read_u16(_file);

        auto const num_segments = map->seg_count_x2 / 2;
        auto const num_to_read = map->seg_count_x2 * 2 + 1;
        for (int i = 0; i < num_to_read; ++i)
          map->segments.segment_data.push_back(read_u16(_file));

        auto const num_glyphs = length / sizeof(uint16_t) - (7 + num_to_read);
        auto const base_of_glyphs = map->segments.segment_data.size();
        map->segments.segment_data.resize(base_of_glyphs + num_glyphs);

        map->segments.end_codes = std::span(map->segments.segment_data).subspan(0, num_segments);
        map->segments.start_codes = std::span(map->segments.segment_data).subspan(num_segments + 1, num_segments);
        map->segments.id_delta = std::span(map->segments.segment_data).subspan(2 * num_segments + 1, num_segments);
        map->segments.id_range_offset_extended = std::span(map->segments.segment_data).subspan(3 * num_segments + 1);

        for (int i = 0; i < num_glyphs; ++i)
          map->segments.segment_data[i + base_of_glyphs] = read_u16(_file);
        _glyph_map = std::move(map);
      }
    }

    // read maxp
    {
      constexpr static auto const tag = make_tag("maxp");
      _file.seekg(_tables[tag].offset);
      _maxp.version_major = read_u16(_file);
      _maxp.version_minor = read_u16(_file);
      _maxp.num_glyphs = read_u16(_file);
      _maxp.max_points = read_u16(_file);
      _maxp.max_contours = read_u16(_file);
      _maxp.max_component_points = read_u16(_file);
      _maxp.max_component_contours = read_u16(_file);
      _maxp.max_zones = read_u16(_file);
      _maxp.max_twilight_points = read_u16(_file);
      _maxp.max_storage = read_u16(_file);
      _maxp.max_function_defs = read_u16(_file);
      _maxp.max_stack_elements = read_u16(_file);
      _maxp.max_size_of_instructions = read_u16(_file);
      _maxp.max_component_elements = read_u16(_file);
      _maxp.max_component_depth = read_u16(_file);
    }

    // read loca
    {
      constexpr static auto const tag = make_tag("loca");
      _file.seekg(_tables[tag].offset);
      _glyph_offsets.resize(_maxp.num_glyphs + 1);
      if (_header.index_to_loc_format == 0)
      {
        for (int i = 0; i < _maxp.num_glyphs + 1; ++i)
        {
          _glyph_offsets[i] = 2 * read_u16(_file);
        }
      }
      else
      {
        for (int i = 0; i < _maxp.num_glyphs + 1; ++i)
        {
          _glyph_offsets[i] = read_u32(_file);
        }
      }
    }
  }
  
  float goop::font_file::units_per_em() const
  {
    return _header.units_per_em;
  }

  std::vector<lines::line_segment> font_file::outline(glyph_id glyph, rect* bounds) const
  {
    constexpr static auto const tag = make_tag("glyf");
    _file.seekg(_tables.at(tag).offset + glyph_offset_bytes(glyph));

    std::int16_t const contours = read_u16(_file);
    std::int16_t const x_min = read_u16(_file);
    std::int16_t const y_min = read_u16(_file);
    std::int16_t const x_max = read_u16(_file);
    std::int16_t const y_max = read_u16(_file);

    if (bounds)
    {
      bounds->min.x = x_min;
      bounds->min.y = y_min;
      bounds->max.x = x_max;
      bounds->max.y = y_max;
    }

    if (contours >= 0)
    {
      // simple glyph

      std::vector<std::uint16_t> end_points(contours);
      for (auto& e : end_points)
        e = read_u16(_file);
      std::uint16_t const instruction_length = read_u16(_file);
      std::vector<std::uint8_t> instructions(instruction_length);
      for (auto& i : instructions)
        i = read_u8(_file);

      static constexpr auto flag_on_curve = 1<<0;
      static constexpr auto flag_x_short = 1<<1;
      static constexpr auto flag_y_short = 1<<2;
      static constexpr auto flag_repeat = 1<<3;
      static constexpr auto flag_x_is_same_or_positive_x_short_vector = 1<<4;
      static constexpr auto flag_y_is_same_or_positive_y_short_vector = 1<<5;

      struct contour_point
      {
        std::uint8_t flags;
        bool on_path;
        std::int16_t x;
        std::int16_t y;
      };
      std::vector<contour_point> contour;
      contour.reserve(end_points.back() + 1);

      int start_point = 0;
      for (int i = 0; i < contours; ++i)
      {
        for (int m = start_point; m <= end_points[i]; ++m)
        {
          // first read flag, then determine how data is read.
          auto& next = contour.emplace_back();
          next.flags = read_u8(_file);
          next.on_path = next.flags & flag_on_curve;
          if (next.flags & flag_repeat)
          {
            auto const times = read_u8(_file);
            for (int rep = 0; rep < times; ++rep)
            {
              ++m;
              contour.push_back(next);
            }
          }
        }
        start_point = end_points[i]+1;
      }

      for (auto& c : contour)
      {
        if (c.flags & flag_x_short)
        {
          auto const sign = ((c.flags & flag_x_is_same_or_positive_x_short_vector) != 0) ? 1 : -1;
          auto const x = read_u8(_file);
          c.x = sign * std::int16_t(x);
        }
        else if(c.flags & flag_x_is_same_or_positive_x_short_vector)
        {
          c.x = 0;
        }
        else
        {
          c.x = read_u16(_file);
        }
      }

      std::int16_t prev_y = 0;
      for (auto& c : contour)
      {
        if (c.flags & flag_y_short)
        {
          auto const sign = ((c.flags & flag_y_is_same_or_positive_y_short_vector) != 0) ? 1 : -1;
          auto const y = read_u8(_file);
          c.y = sign * std::int16_t(y);
        }
        else if (c.flags & flag_y_is_same_or_positive_y_short_vector)
        {
          c.y = 0;
        }
        else
        {
          c.y = read_u16(_file);
        }
      }

      struct absolute_point
      {
        bool on_line;
        rnu::vec2 position;
      };

      rnu::vec2 point{ 0, 0 };
      std::vector<absolute_point> lines;
      lines.reserve(contour.size());
      for (auto const& c : contour)
      {
        point.x += c.x;
        point.y += c.y;
        lines.push_back({ c.on_path, point });
      }

      std::vector<lines::line_segment> segments;
      {

        int start_point = 0;
        for (auto const end_point : end_points)
        {
          auto points = std::span(lines).subspan(start_point, end_point + 1 - start_point);
          auto const previous = [&](size_t index) { 
            return (index + points.size() - 1) % points.size();
          };
          auto const get_previous_on_line = [&](size_t index)
          {
            auto const& prev_point = points[previous(index)];
            auto const& this_point = points[index];

            // prev on line ? ret prev
            // prev not on line ? ret (prev + this) / 2;

            if (prev_point.on_line)
              return prev_point.position;
            else
              return (prev_point.position + this_point.position) / 2.0f;
          };

          bool first = true;
          bool last_was_on_line;
          rnu::vec2 last_point_on_line;
          rnu::vec2 last_control_point;

          size_t count = points.size();
          for (size_t i = 0; i <= count; ++i)
          {
            auto const wrapping_index = i % points.size();
            auto const& point = points[wrapping_index];
            auto const this_was_on_line = point.on_line;
            auto const this_point_on_line = point.on_line ? point.position : get_previous_on_line(wrapping_index);
            auto const this_control_point = point.position;

            if (!first)
            {
              if (last_was_on_line && this_was_on_line)
              {
                segments.push_back(lines::line{
                  .start = last_control_point,
                  .end = this_control_point
                  });
              }
              else
              {
                segments.push_back(lines::bezier{
                  .start = last_point_on_line,
                  .control = last_control_point,
                  .end = this_point_on_line
                  });
              }
            }
            else
            {
              first = false;
            }
            last_was_on_line = this_was_on_line;
            last_point_on_line = this_point_on_line;
            last_control_point = this_control_point;
          }

          start_point = end_point + 1;
        }
      }
      return segments;
    }
    else
    {
      // compound glyph
    }

    __debugbreak();
    return {};
  }
}