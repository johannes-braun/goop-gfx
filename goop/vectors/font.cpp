#include "font.hpp"
#include <span>
#include <ranges>
#include <array>

namespace goop
{
  void DEBUGME()
  {
    __debugbreak(); // Todo: test what happens here...
  }



#pragma region FontAccessor
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

  font_accessor::font_accessor(std::filesystem::path const& path)
    : _file{ path, std::ios::binary }
  {
    // read offset subtable
    _offsets.scaler = read_u32(_file);
    _offsets.num_tables = read_u16(_file);
    _offsets.search_range = read_u16(_file);
    _offsets.entry_selector = read_u16(_file);
    _offsets.range_shift = read_u16(_file);

    seek_table(make_tag("hhea"));
    s_u32(); // version
    _hhea.ascent = r_u16();
    _hhea.descent = r_u16();
    _hhea.line_gap = r_u16();
    _hhea.advance_width_max = r_u16();
    _hhea.min_left_side_bearing = r_u16();
    _hhea.min_right_side_bearing = r_u16();
    _hhea.x_max_extent = r_u16();
    _hhea.caret_slope_rise = r_u16();
    _hhea.caret_slope_run = r_u16();
    _hhea.caret_offset = r_u16();
    // 4x 16 bits reserved
    skip(4 * sizeof(std::uint16_t));
    _hhea.metric_data_format = r_u16();
    _hhea.num_long_horizontal_metrics = r_u16();

    _hmtx_offset = *table_offset(make_tag("hmtx"));

    seek_table(make_tag("maxp"));
    s_u32(); // version
    _num_glyphs = r_u16();

    seek_table(make_tag("head"));
    s_u32(); // version
    s_u32(); // revision
    _header.checksum_adjustment = r_u32();
    auto const magic_number = r_u32();
    if (magic_number != ttf_magic_number)
      throw std::runtime_error("Error while reading font: invalid magic number.");

    _header.flags = r_u16();
    _header.units_per_em = r_u16();
    s_u64(); // created
    s_u64(); // modified
    _header.x_min = r_u16();
    _header.y_min = r_u16();
    _header.x_max = r_u16();
    _header.y_max = r_u16();
    s_u16(); // mac_style
    _header.lowest_recommended_ppem = r_u16();
    _header.font_direction_hint = r_u16();
    _header.index_to_loc_format = loc_format{ r_u16() };
    _header.glyph_data_format = r_u16();

    auto const offset = seek_table(make_tag("cmap"));
    std::uint16_t version = r_u16();
    std::uint16_t num_subtables = r_u16();

    for (int i = 0; i < num_subtables; ++i)
    {
      std::uint16_t platform_id = r_u16();
      std::uint16_t encoding_id = r_u16();
      std::uint32_t subtable_offset = r_u32();

      if (platform_id == 0) // unicode
      {
        seek_to(*offset + subtable_offset);
        std::uint16_t const format = r_u16();
        std::uint16_t const length = r_u16();
        std::uint16_t const language = r_u16();

        if (format == 0)
        {
          _glyph_indexer = glyph_index_data_f0{ .offset = *offset + subtable_offset };
        }
        else if (format == 4)
        {
          glyph_index_data_f4 format4;
          format4.offset = *offset + subtable_offset;
          format4.seg_count_x2 = r_u16();
          format4.search_range = r_u16();
          format4.entry_selector = r_u16();
          format4.range_shift = r_u16();
          _glyph_indexer = format4;
        }
        else
        {
          throw std::runtime_error("Detected glyph cmap format not supported.");
        }

        break;
      }
    }

    // GPOS
    if (auto const gpos_base = seek_table(make_tag("GPOS")))
    {
      _gpos_off = gpos_off{};

      auto const major = r_u16();
      auto const minor = r_u16();

      _gpos_off->script_list = gpos_base.value() + r_u16();
      _gpos_off->feature_list = gpos_base.value() + r_u16();
      _gpos_off->lookup_list = gpos_base.value() + r_u16();
      if (major == 1 && minor == 1)
        _gpos_off->feature_variations = gpos_base.value() + r_u32();
    }
  }

  font_accessor::horizontal_metric font_accessor::hmetric(glyph_id glyph)
  {
    auto const glyph_id = std::uint32_t(glyph);
    if (glyph_id < _hhea.num_long_horizontal_metrics)
    {
      seek_to(_hmtx_offset + 2 * sizeof(std::uint16_t) * glyph_id);
      horizontal_metric metric;
      metric.advance_width = r_u16();
      metric.left_bearing = r_u16();
      return metric;
    }
    else
    {
      horizontal_metric metric;
      seek_to(_hmtx_offset + 2 * sizeof(std::uint16_t) * (_hhea.num_long_horizontal_metrics - 1));
      metric.advance_width = r_u16();
      seek_to(_hmtx_offset + sizeof(std::uint16_t) * (2 * _hhea.num_long_horizontal_metrics + glyph_id));
      metric.left_bearing = r_u16();
      return metric;
    }
  }

  constexpr bool glyph_id_less(glyph_id g, glyph_id index)
  {
    return std::uint32_t(g) < std::uint32_t(index);
  };

  std::size_t font_accessor::units_per_em() const
  {
    return _header.units_per_em;
  }

  std::optional<glyph_class> font_accessor::class_of(std::size_t class_table, glyph_id glyph)
  {
    auto const prev = position();
    seek_to(class_table);
    auto const format = r_u16();
    auto const glyph_cur = std::uint16_t(glyph);

    if (format == 1)
    {
      auto const start_glyph = r_u16();
      auto const num_glyphs = r_u16();

      if (glyph_cur < start_glyph || glyph_cur >= start_glyph + num_glyphs)
        return std::nullopt;

      auto const index = start_glyph - glyph_cur;
      seek_to(position() + index * sizeof(std::uint16_t));
      auto const class_val = r_u16();
      seek_to(prev);
      return glyph_class{ class_val }; 
    }
    else if (format == 2)
    {
      auto const num_ranges = r_u16();
      auto const base_offset = position();

      auto const map = [&](int k) {
        seek_to(base_offset + k * 3 * sizeof(std::uint16_t) + sizeof(std::uint16_t));
        return glyph_id(r_u16());
      };

      auto const bound = std::ranges::upper_bound(std::ranges::views::iota(0, int(num_ranges)), glyph, &glyph_id_less, map);
      auto const found_index = *bound;

      seek_to(base_offset + found_index * 3 * sizeof(std::uint16_t) );
      auto const lower = r_u16();
      auto const upper = r_u16();
      auto const class_val = r_u16();

      if (glyph_cur < lower || glyph_cur > upper)
        return std::nullopt;

      return glyph_class{ class_val };
    }
    else
    {
      throw std::runtime_error("Invalid class format");
    }
  }

  std::optional<glyph_id> font_accessor::index_of(char32_t character)
  {
    if (auto const* f0 = std::get_if<glyph_index_data_f0>(&_glyph_indexer))
    {
      if (character < 0 || character > 255)
        return std::nullopt;
      seek_to(f0->offset + 3 * sizeof(std::uint16_t) + character * sizeof(std::uint8_t));
      return glyph_id(r_u8());
    }
    else if (auto const* f4 = std::get_if<glyph_index_data_f4>(&_glyph_indexer))
    {
      auto const base_offset = f4->offset + 3 * sizeof(std::uint16_t);
      auto const end_points_offset = base_offset + 4 * sizeof(std::uint16_t);
      auto const end_point = [&](size_t offset) {
        seek_to(end_points_offset + offset);
        return r_u16();
      };

      auto begin = 0;
      auto center = f4->search_range >> 1;
      auto end = f4->search_range;

      // binary search
      for (int i = 1; i < f4->entry_selector; ++i)
      {
        if (character < end_point(center))
        {
          end = center;
          center -= (f4->search_range >> (1 + i));
        }
        else
        {
          begin = center;
          center += (f4->search_range >> (1 + i));
        }
      }

      if (character > end_point(center))
      {
        center = end;
      }

      auto const bound_upper = end_point(center);
      auto const u16_offset_bytes = center;
      auto const sized_seg_count = sizeof(std::uint16_t) * (f4->seg_count_x2 / 2);
      auto const start_codes_offset = end_points_offset + sizeof(std::uint16_t) * (f4->seg_count_x2 / 2 + 1);
      seek_to(start_codes_offset + u16_offset_bytes);
      auto const bound_lower = r_u16();

      if (character < bound_lower || character > bound_upper)
        return std::nullopt;

      seek_to(start_codes_offset + sized_seg_count + u16_offset_bytes);
      auto const id_delta = r_u16();
      seek_to(start_codes_offset + sized_seg_count + sized_seg_count + u16_offset_bytes);
      auto const id_range_offset = r_u16();

      if (id_range_offset == 0)
        return glyph_id{ (id_delta + character) % 65536 };

      seek_to(start_codes_offset + sized_seg_count + sized_seg_count + u16_offset_bytes +
        id_range_offset + (character - bound_lower) * sizeof(std::uint16_t));
      return glyph_id(r_u16());
    }
    else
    {
      throw std::runtime_error("Detected glyph cmap format not supported.");
    }
  }

  std::optional<std::size_t> font_accessor::table_offset(font_tag tag)
  {
    seek_to(sizeof(offset_subtable));
    auto const table_id = [&](size_t offset) {
      seek_to(sizeof(offset_subtable) + offset);
      return r_tag();
    };
    auto const less = [&](font_tag t) {
      return std::uint32_t(tag) < std::uint32_t(t);
    };
    auto const greater = [&](font_tag t) {
      return std::uint32_t(tag) > std::uint32_t(t);
    };

    if (less(table_id(_offsets.range_shift)))
    {
      // search shift linearly
      for (int i = 0; i < _offsets.range_shift; i += table_size)
      {
        if (tag == table_id(i))
        {
          s_u32(); // checksum
          return r_u32(); // offset
        }
      }
    }
    else
    {
      auto begin = 0;
      auto center = _offsets.search_range >> 1;
      auto end = _offsets.search_range;

      // binary search
      for (int i = 1; i < _offsets.entry_selector; ++i)
      {
        if (less(table_id(center)))
        {
          end = center;
          center -= (_offsets.search_range >> (1 + i));
        }
        else
        {
          begin = center;
          center += (_offsets.search_range >> (1 + i));
        }
      }

      if (greater(table_id(center)))
      {
        center = end;
      }
      else if (less(table_id(center)))
      {
        center = begin;
      }

      if (tag == table_id(center))
      {
        s_u32(); // checksum
        return r_u32(); // offset
      }
      return std::nullopt;
    }

    return std::nullopt;
  }

  std::size_t font_accessor::glyph_offset_bytes(glyph_id glyph)
  {
    auto const glyph_num = std::uint32_t(glyph);
    if (_header.index_to_loc_format == loc_format::short_int16)
    {
      seek_to(*table_offset(make_tag("loca")) + glyph_num * sizeof(std::uint16_t));
      return std::size_t{ r_u16() } *2;
    }
    else
    {
      seek_to(*table_offset(make_tag("loca")) + glyph_num * sizeof(std::uint32_t));
      return r_u32();
    }
  }

  std::optional<std::uint16_t> font_accessor::script_offset(font_script script)
  {
    if (!_gpos_off)
      return std::nullopt;

    seek_to(_gpos_off->script_list);

    std::uint16_t num_scripts = r_u16();

    struct script_ref
    {
      char tag[4];
      std::int16_t script_offset;
    };

    for (int i = 0; i < num_scripts; ++i)
    {
      if (font_script{r_u32()} == script)
        return r_u16();
      s_u16();
    }
    return std::nullopt;
  }

  std::optional<std::uint16_t> font_accessor::lang_offset(font_language lang, std::uint16_t script)
  {
    if (!_gpos_off)
      return std::nullopt;

    seek_to(_gpos_off->script_list + script);

    auto const default_offset = r_u16();
    auto const count = r_u16();

    for (int i = 0; i < count; ++i)
    {
      if (font_language{ r_u32() } == lang)
        return r_u16();
      s_u16();
    }

    if (default_offset == 0)
      return std::nullopt;
    return default_offset;
  }

  std::optional<std::uint16_t> font_accessor::feature_offset(font_feature feat, std::uint16_t lang)
  {
    if (!_gpos_off)
      return std::nullopt;

    seek_to(_gpos_off->script_list + lang);

    // search in featureIndices for feature with the given tag
    s_u16(); // lookupOrderOffset
    s_u16(); // required feature index
    std::uint16_t const feature_index_count = r_u16();

    for (int i = 0; i < feature_index_count; ++i)
    {
      auto const index = r_u16();
      auto const next = position();

      seek_to(_gpos_off->feature_list + sizeof(std::uint16_t) + index * (4 * sizeof(std::byte) + sizeof(std::int16_t)));
      if (feat == font_feature{ r_u32() })
      {
        return r_u16();
      }
      seek_to(next);
    }
    return std::nullopt;
  }

  std::optional<std::size_t> font_accessor::coverage_index(std::size_t offset, glyph_id glyph)
  {
    auto const prev = position();
    seek_to(offset);

    auto const format = r_u16();
    if (format == 1)
    {
      auto const count = r_u16();
      auto const base_offset = position();

      auto const map = [&](int k) {
        seek_to(base_offset + k * sizeof(std::uint16_t));
        return glyph_id(r_u16());
      };

      auto const bound = std::ranges::lower_bound(std::ranges::views::iota(0, int(count)), glyph, &glyph_id_less, map);

      auto const found_index = *bound;
      auto const g = map(found_index);

      seek_to(prev);

      DEBUGME();

      if (g == glyph)
        return found_index;
      return std::nullopt;
    }
    else if (format == 2)
    {
      auto const count = r_u16();
      auto const base_offset = position();

      auto const map = [&](int k) {
        seek_to(base_offset + k * (sizeof(std::uint16_t) * 3) + sizeof(std::uint16_t));
        return glyph_id(r_u16());
      };

      auto const bound = std::ranges::upper_bound(std::ranges::views::iota(0, int(count)), glyph, &glyph_id_less, map);
      auto const found_index = *bound;

      seek_to(base_offset + found_index * (sizeof(std::uint16_t) * 3));
      auto const lower = r_u16();
      auto const upper = r_u16();
      auto const start_index = r_u16();
      std::uint32_t const g = std::uint32_t(glyph);

      if (g < lower || g > upper)
        return std::nullopt;

      seek_to(prev);

      return start_index + (g - lower);
    }
    else
    {
      throw std::runtime_error("Unknown coverage format");
    }

    seek_to(prev);
    return std::nullopt;
  }

  constexpr static auto val_flag_x_placement = 0x0001;
  constexpr static auto val_flag_y_placement = 0x0002;
  constexpr static auto val_flag_x_advance = 0x0004;
  constexpr static auto val_flag_y_advance = 0x0008;
  constexpr static auto val_flag_x_placement_device = 0x0010;
  constexpr static auto val_flag_y_placement_device = 0x0020;
  constexpr static auto val_flag_x_advance_device = 0x0040;
  constexpr static auto val_flag_y_advance_device = 0x0080;

  size_t value_record_size(std::uint16_t flags)
  {
    int count = 0;
    count += (flags & val_flag_x_placement) != 0;
    count += (flags & val_flag_y_placement) != 0;
    count += (flags & val_flag_x_advance) != 0;
    count += (flags & val_flag_y_advance) != 0;
    count += (flags & val_flag_x_placement_device) != 0;
    count += (flags & val_flag_y_placement_device) != 0;
    count += (flags & val_flag_x_advance_device) != 0;
    count += (flags & val_flag_x_advance_device) != 0;
    return count * sizeof(std::uint16_t);
  }

  font_accessor::value_record font_accessor::r_value(std::uint16_t flags)
  {
    value_record rec{};
    if ((flags & val_flag_x_placement) != 0)
      rec.x_placement = r_u16();
    if ((flags & val_flag_y_placement) != 0)
      rec.y_placement = r_u16();
    if ((flags & val_flag_x_advance) != 0)
      rec.x_advance = r_u16();
    if ((flags & val_flag_y_advance) != 0)
      rec.y_advance = r_u16();
    if ((flags & val_flag_x_placement_device) != 0)
      rec.x_placement_device_off = r_u16();
    if ((flags & val_flag_y_placement_device) != 0)
      rec.y_placement_device_off = r_u16();
    if ((flags & val_flag_x_advance_device) != 0)
      rec.x_advance_device_off = r_u16();
    if ((flags & val_flag_y_advance_device) != 0)
      rec.y_advance_device_off = r_u16();
    return rec;
  }

  font_accessor::feature font_accessor::feature_lookup(std::uint16_t feat, std::span<glyph_id const> glyphs)
  {
    constexpr static std::monostate nullfeature{};

    if (!_gpos_off)
      return nullfeature;

    seek_to(_gpos_off->feature_list + feat);
    s_u16(); // feature params offset
    auto const count = r_u16();

    for (int i = 0; i < count; ++i)
    {
      auto const index = r_u16();
      auto const next = position();

      seek_to(_gpos_off->lookup_list + sizeof(std::uint16_t) + index * sizeof(std::int16_t));
      auto const lookup_offset = r_u16();
      seek_to(_gpos_off->lookup_list + lookup_offset);

      enum class lookup_type_t
      {
        single_adj = 1,
        pair_adj = 2,
        cursive_attachment_pos = 3,
        mark_to_base_attachment_pos = 4,
        mark_to_ligature_attachment_pos = 5,
        mark_to_mark_attachment_pos = 6,
        contextual_pos = 7,
        chained_contexts_pos = 8,
        extension_pos = 9,
      };

      auto const lookup_type = lookup_type_t{ r_u16() };
      auto const lookup_flag = r_u16();
      auto const num_subtables = r_u16();

      for (int sub = 0; sub < num_subtables; ++sub)
      {
        auto const subtable_offset = r_u16();
        auto const next_subtable_offset = position();

        seek_to(_gpos_off->lookup_list + lookup_offset + subtable_offset);

        switch (lookup_type)
        {
        case lookup_type_t::single_adj:
        {
          if (glyphs.size() != 1)
          {
            // we did not ask for a one-glyph-thing...
            break;
          }

          auto const fmt = r_u16();
          auto const coverage_offset = r_u16();
          auto const glyph = std::uint32_t(glyphs[0]);

          auto covered = coverage_index(_gpos_off->lookup_list + lookup_offset + subtable_offset + coverage_offset, glyphs[0]);

          DEBUGME();
          if (covered)
          {
            auto const value_format = r_u16();

            if (fmt == 1)
            {
              // All have the same value format
              return r_value(value_format);
            }
            else
            {
              // We have one item per covered glyph
              auto const rec_size = value_record_size(value_format);
              s_u16(); // value_count
              seek_to(position() + rec_size * *covered);
              return r_value(value_format);
            }
          }
          // Otherwise try next subtable
        }
        break;
        case lookup_type_t::pair_adj:
        {
          if (glyphs.size() != 2)
          {
            // we did not ask for a two-glyph-thing...
            break;
          }

          auto const base_off = position();
          auto const fmt = r_u16();
          std::int16_t const coverage_offset = r_u16();

          auto covered = coverage_index(_gpos_off->lookup_list + lookup_offset + subtable_offset + coverage_offset, glyphs[0]);

          if (!covered)
            break; // break out of switch (not of loop)

          auto const value_format_1 = r_u16();
          auto const value_format_2 = r_u16();
          auto const record_size_1 = value_record_size(value_format_1);
          auto const record_size_2 = value_record_size(value_format_2);

          if (fmt == 1)
          {
            auto const num_pair_sets = r_u16();
            seek_to(position() + covered.value() * sizeof(std::uint16_t));
            auto const pair_set_offset = r_u16();
            seek_to(_gpos_off->lookup_list + lookup_offset + subtable_offset + pair_set_offset);
            auto const pair_value_count = r_u16();
            auto const base_offset = position();

            auto const pair_value_size = sizeof(std::uint16_t) +
              record_size_1 + record_size_2;

            auto const map = [&](int k) {
              seek_to(base_offset + k * pair_value_size);
              return glyph_id(r_u16());
            };

            auto const bound = std::ranges::lower_bound(std::ranges::views::iota(0, int(pair_value_count)), glyphs[1], &glyph_id_less, map);
            auto const found_index = *bound;
            auto const mp = map(found_index-1);
            seek_to(base_offset + found_index * pair_value_size);
            auto const other_glyph = r_u16(); // glyph
            if (std::uint32_t(other_glyph) != std::uint32_t(glyphs[1]))
              break; // not covered

            auto const value_1 = r_value(value_format_1);
            auto const value_2 = r_value(value_format_2);

            return pair_feature(value_1, value_2);
          }
          else if (fmt == 2)
          {
            auto const class_def1_off = r_u16();
            auto const class_def2_off = r_u16();
            auto const class_def1_count = r_u16();
            auto const class_def2_count = r_u16();
            auto const base_of_class1_recs = position();

            auto const class1 = class_of(_gpos_off->lookup_list + lookup_offset + subtable_offset + class_def1_off, glyphs[0]);
            auto const class2 = class_of(_gpos_off->lookup_list + lookup_offset + subtable_offset + class_def2_off, glyphs[1]);

            if (!class1 || !class2)
              break; // not all are classed.

            auto const class1_int = std::uint16_t(class1.value());
            auto const class2_int = std::uint16_t(class2.value());

            auto const class2_rec_size = record_size_1 + record_size_2;
            auto const record = class_def2_count * class2_rec_size * class1_int + class2_rec_size * class2_int;

            seek_to(_gpos_off->lookup_list + lookup_offset + subtable_offset + base_of_class1_recs + record);

            auto const value_1 = r_value(value_format_1);
            auto const value_2 = r_value(value_format_2);
            return pair_feature(value_1, value_2);
          }
          else
          {
            throw std::runtime_error("Invalid PairPosFormat");
          }
          // Otherwise try next subtable
        }
        break;
        case lookup_type_t::cursive_attachment_pos:
        case lookup_type_t::mark_to_base_attachment_pos:
        case lookup_type_t::mark_to_ligature_attachment_pos:
        case lookup_type_t::mark_to_mark_attachment_pos:
        case lookup_type_t::contextual_pos:
        case lookup_type_t::chained_contexts_pos:
        case lookup_type_t::extension_pos:
        default:
          throw std::runtime_error("Not implemented");
          break;
        }

        seek_to(next_subtable_offset);
      }

      seek_to(next);
    }

    return nullfeature;
  }

  std::optional<std::size_t> font_accessor::seek_table(font_tag tag)
  {
    auto const offset = table_offset(tag);
    if (offset) seek_to(offset.value());
    return offset;
  }

  void font_accessor::seek_to(std::size_t offset)
  {
    _file.seekg(offset);
  }

  std::size_t font_accessor::position()
  {
    return _file.tellg();
  }

  std::uint8_t font_accessor::r_u8()
  {
    return read_u8(_file);
  }

  std::uint16_t font_accessor::r_u16()
  {
    return read_u16(_file);
  }

  std::uint32_t font_accessor::r_u32()
  {
    return read_u32(_file);
  }

  std::uint64_t font_accessor::r_u64()
  {
    return read_u64(_file);
  }

  font_tag font_accessor::r_tag()
  {
    return font_tag{ read_u32(_file) };
  }

  void font_accessor::s_u8()
  {
    skip(sizeof(std::uint8_t));
  }

  void font_accessor::s_u16()
  {
    skip(sizeof(std::uint16_t));
  }

  void font_accessor::s_u32()
  {
    skip(sizeof(std::uint32_t));
  }

  void font_accessor::s_u64()
  {
    skip(sizeof(std::uint64_t));
  }

  void font_accessor::skip(std::size_t bytes)
  {
    _file.seekg(bytes, std::ios::cur);
  }

#pragma endregion

  std::optional<glyph_id> font_file::glyph_index(char32_t character) const
  {
    return _accessor.index_of(character);
  }


  std::pair<float, float> font_file::advance_bearing(glyph_id current, glyph_id next)
  {
    auto const hmetric = _accessor.hmetric(current);
    auto const kerning = lookup_kerning(current, next);

    if (kerning)
      return { hmetric.advance_width + kerning.value().first.x_advance, hmetric.left_bearing };
    return { hmetric.advance_width, hmetric.left_bearing };
  }

  font_file::font_file(std::filesystem::path const& path)
    : _accessor{ path }
  {
    try_load_gpos();
  }
  
  void font_file::try_load_gpos()
  {
    auto script_offset = _accessor.script_offset(font_script::default_script);

    if (!script_offset)
    {
      throw std::runtime_error("Neither latn nor DFLT have been found");
      return;
    }

    auto const lang_offset = _accessor.lang_offset(font_language::default_language, *script_offset);
    if (!lang_offset)
    {
      throw std::runtime_error("The lang was not found");
      return;
    }

    auto const kerning = _accessor.feature_offset(font_feature::ft_kern, *script_offset + *lang_offset);
    _gpos_features.kerning = kerning;
  }

  std::optional<font_accessor::pair_feature> font_file::lookup_kerning(glyph_id first, glyph_id second) const
  {
    if (!_gpos_features.kerning)
      return std::nullopt;

    auto const lu = _accessor.feature_lookup(_gpos_features.kerning.value(), std::array{ first, second });
    auto const vals = std::get_if<font_accessor::pair_feature>(&lu);
    if (vals)
      return *vals;
    return std::nullopt;
  }

  float goop::font_file::units_per_em() const
  {
    return _accessor.units_per_em();
  }

  void font_file::outline_impl(glyph_id glyph, std::vector<font_file::contour_point>& contours, std::vector<std::uint16_t>& end_points, rect* bounds) const
  {
    constexpr static auto const tag = make_tag("glyf");
    _accessor.seek_to(*_accessor.table_offset(tag) + _accessor.glyph_offset_bytes(glyph));

    std::int16_t const num_contours = _accessor.r_u16();
    std::int16_t const x_min = _accessor.r_u16();
    std::int16_t const y_min = _accessor.r_u16();
    std::int16_t const x_max = _accessor.r_u16();
    std::int16_t const y_max = _accessor.r_u16();

    if (bounds)
    {
      bounds->min.x = x_min;
      bounds->min.y = y_min;
      bounds->max.x = x_max;
      bounds->max.y = y_max;
    }

    auto const base_end_point_count = end_points.size();
    auto const base_end_point = end_points.empty() ? 0 : contours.size();
    if (num_contours >= 0)
    {
      // simple glyph
      for (int i = 0; i < num_contours; ++i)
        end_points.push_back(base_end_point + _accessor.r_u16());
      std::uint16_t const instruction_length = _accessor.r_u16();
      _accessor.skip(instruction_length * sizeof(std::uint8_t));

      static constexpr auto flag_on_curve = 1 << 0;
      static constexpr auto flag_x_short = 1 << 1;
      static constexpr auto flag_y_short = 1 << 2;
      static constexpr auto flag_repeat = 1 << 3;
      static constexpr auto flag_x_is_same_or_positive_x_short_vector = 1 << 4;
      static constexpr auto flag_y_is_same_or_positive_y_short_vector = 1 << 5;

      int start_point = base_end_point;
      int const start_contour = contours.size();
      for (int i = 0; i < num_contours; ++i)
      {
        for (int m = start_point; m <= end_points[base_end_point_count + i]; ++m)
        {
          // first read flag, then determine how data is read.
          auto& next = contours.emplace_back();
          next.flags = _accessor.r_u8();
          next.on_line = next.flags & flag_on_curve;
          if (next.flags & flag_repeat)
          {
            auto const times = _accessor.r_u8();
            for (int rep = 0; rep < times; ++rep)
            {
              ++m;
              contours.push_back(next);
            }
          }
        }
        start_point = end_points[base_end_point_count + i] + 1;
      }

      for (int m = start_contour; m < contours.size(); ++m)
      {
        auto& c = contours[m];
        if (c.flags & flag_x_short)
        {
          auto const sign = ((c.flags & flag_x_is_same_or_positive_x_short_vector) != 0) ? 1 : -1;
          auto const x = _accessor.r_u8();
          c.pos.x = sign * std::int16_t(x);
        }
        else if (c.flags & flag_x_is_same_or_positive_x_short_vector)
        {
          c.pos.x = 0;
        }
        else
        {
          c.pos.x = static_cast<std::int16_t>(_accessor.r_u16());
        }
      }

      std::int16_t prev_y = 0;
      for (int m = start_contour; m < contours.size(); ++m)
      {
        auto& c = contours[m];
        if (c.flags & flag_y_short)
        {
          auto const sign = ((c.flags & flag_y_is_same_or_positive_y_short_vector) != 0) ? 1 : -1;
          auto const y = _accessor.r_u8();
          c.pos.y = sign * std::int16_t(y);
        }
        else if (c.flags & flag_y_is_same_or_positive_y_short_vector)
        {
          c.pos.y = 0;
        }
        else
        {
          c.pos.y = static_cast<std::int16_t>(_accessor.r_u16());
        }
      }

      // make all (except first) points absolute
      rnu::vec2 point{ 0, 0 };
      for (int m = start_contour; m < contours.size(); ++m)
      {
        auto& c = contours[m];
        point += c.pos;
        c.pos = point;
      }
    }
    else
    {
      // composite glyph
      constexpr static std::uint16_t flag_arg_1_and_2_are_words = 1 << 0;
      constexpr static std::uint16_t flag_args_are_xy_values = 1 << 1;
      constexpr static std::uint16_t flag_round_xy_to_grid = 1 << 2;
      constexpr static std::uint16_t flag_we_have_scale = 1 << 3;
      constexpr static std::uint16_t flag_more_components = 1 << 5;
      constexpr static std::uint16_t flag_we_have_x_and_y_scale = 1 << 6;
      constexpr static std::uint16_t flag_we_have_a_two_by_two = 1 << 7;
      constexpr static std::uint16_t flag_we_have_instructions = 1 << 8;
      constexpr static std::uint16_t flag_use_my_metrics = 1 << 9;
      constexpr static std::uint16_t flag_overlap_compound = 1 << 10;

      auto flags = flag_more_components;

      int base_shape_size = -1;
      while (flags & flag_more_components)
      {
        flags = _accessor.r_u16();
        auto const index_of_component = _accessor.r_u16();
        auto const arg1 = (flags & flag_arg_1_and_2_are_words) ? _accessor.r_u16() : _accessor.r_u8();
        auto const arg2 = (flags & flag_arg_1_and_2_are_words) ? _accessor.r_u16() : _accessor.r_u8();

        std::int16_t a = 1, b = 0, c = 0, d = 1;

        if (flags & flag_we_have_scale)
        {
          a = _accessor.r_u16();
          d = a;
        }
        else if (flags & flag_we_have_x_and_y_scale)
        {
          a = _accessor.r_u16();
          d = _accessor.r_u16();
        }
        else if (flags & flag_we_have_a_two_by_two)
        {
          a = _accessor.r_u16();
          b = _accessor.r_u16();
          c = _accessor.r_u16();
          d = _accessor.r_u16();
        }

        float m = 0;
        float n = 0;
        auto m0 = std::max(abs(a), abs(b));
        if (abs(abs(a) - abs(c)) <= 33 / 65536)
          m = 2 * m0;
        else
          m = m0;
        auto n0 = std::max(abs(c), abs(d));
        if (abs(abs(b) - abs(d)) <= 33 / 65536)
          n = 2 * n0;
        else
          n = n0;

        size_t const pos = _accessor.position();

        auto const shape_begin = contours.size();
        rect unused_rect;
        outline_impl(glyph_id{ index_of_component }, contours, end_points, &unused_rect);
        auto const shape_end = contours.size();

        auto const shape_points = std::span(contours).subspan(shape_begin, shape_end - shape_begin);
        auto const base_points = std::span(contours).subspan(0, base_shape_size);

        float e = 0;
        float f = 0;
        if (flags & flag_args_are_xy_values)
        {
          e = arg1;
          f = arg2;
        }
        else
        {
          auto const pt1 = shape_points[arg1];
          auto const pt2 = base_points[arg2];
          auto const offset = pt2.pos - pt1.pos;
          e = offset.x;
          f = offset.y;
        }

        auto const transform = [&](rnu::vec2 point)
        {
          auto const x = point.x;
          auto const y = point.y;
          point.x = m * ((a / m) * x + (c / m) * y + e);
          point.y = m * ((b / n) * x + (d / n) * y + f);
          return point;
        };
        for (auto& pt : shape_points)
        {
          pt.pos = transform(pt.pos);
        }

        if (base_shape_size == -1)
          base_shape_size = contours.size();

        _accessor.seek_to(pos);
      }
    }
  }

  void font_file::outline(glyph_id glyph, std::vector<lines::line_segment>& segments, rect& bounds) const
  {
    thread_local static std::vector<contour_point> prealloc_contour_buffer;
    prealloc_contour_buffer.clear();
    thread_local static std::vector<std::uint16_t> end_points;
    end_points.clear();

    outline_impl(glyph, prealloc_contour_buffer, end_points, &bounds);

    {
      int start_point = 0;
      for (auto const end_point : end_points)
      {
        auto points = std::span(prealloc_contour_buffer).subspan(start_point, end_point + 1 - start_point);
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
            return prev_point.pos;
          else
            return (prev_point.pos + this_point.pos) / 2.0f;
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
          auto const this_point_on_line = point.on_line ? point.pos : get_previous_on_line(wrapping_index);
          auto const this_control_point = point.pos;

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
  }
}