#include "font.hpp"
#include <span>
#include <ranges>
#include <array>
#include <cassert>

namespace goop
{
  class ptr_reader
  {
  public:
    ptr_reader(std::byte const* data);

    void seek_to(std::ptrdiff_t offset);
    std::size_t position() const;

    std::uint8_t r_u8();
    std::uint16_t r_u16();
    std::uint32_t r_u32();
    std::uint64_t r_u64();

    // skip funcs
    void s_u8();
    void s_u16();
    void s_u32();
    void s_u64();
    void skip(std::size_t bytes);

  private:
    std::byte const* _data = nullptr;
    std::byte const* _read_head = _data;
  };

  ptr_reader::ptr_reader(std::byte const* data)
    : _data{ data }, _read_head{ data }
  {

  }

  void ptr_reader::seek_to(std::ptrdiff_t offset)
  {
    _read_head = std::next(_data, offset);
  }

  std::size_t ptr_reader::position() const
  {
    return std::distance(_data, _read_head);
  }

  std::uint8_t ptr_reader::r_u8()
  {
    std::uint8_t res{};
    std::memcpy(&res, _read_head, sizeof(res));
    s_u8();
    return res;
  }

  std::uint16_t ptr_reader::r_u16()
  {
    if constexpr (std::endian::native != std::endian::big)
      return (std::uint16_t(r_u8()) << 8) | r_u8();
    else
      return r_u8() | (std::uint16_t(r_u8()) << 8);
  }

  std::uint32_t ptr_reader::r_u32()
  {
    if constexpr (std::endian::native != std::endian::big)
      return (std::uint32_t(r_u16()) << 16) | r_u16();
    else
      return r_u16() | (std::uint32_t(r_u16()) << 16);
  }

  std::uint64_t ptr_reader::r_u64()
  {
    if constexpr (std::endian::native != std::endian::big)
      return (std::uint64_t(r_u32()) << 32) | r_u32();
    else
      return r_u32() | (std::uint64_t(r_u32()) << 32);
  }

  // skip funcs
  void ptr_reader::s_u8()
  {
    skip(sizeof(uint8_t));
  }

  void ptr_reader::s_u16()
  {
    skip(sizeof(std::uint16_t));
  }

  void ptr_reader::s_u32()
  {
    skip(sizeof(std::uint32_t));
  }

  void ptr_reader::s_u64()
  {
    skip(sizeof(std::uint64_t));
  }

  void ptr_reader::skip(std::size_t bytes)
  {
    std::advance(_read_head, bytes);
  }

  ptr_reader font_accessor::begin_read() const
  {
    return ptr_reader(std::visit([](auto& d) {return std::data(d); }, _file_data));
  }

  std::optional<font_accessor::gspec_off> const& font_accessor::gpos() const
  {
    return _gpos_off;
  }

  std::optional<font_accessor::gspec_off> const& font_accessor::gsub() const
  {
    return _gsub_off;
  }

  std::size_t font_accessor::num_glyphs() const
  {
    return _maxp.num_glyphs;
  }

  font_accessor::font_accessor(std::span<std::byte const> data, bool copy)
    : _file_data(copy ? std::vector(begin(data), end(data)) : data)
  {
    init();
  }

  font_accessor::font_accessor(std::filesystem::path const& path)
  {
    std::ifstream file(path, std::ios::binary);
    file.seekg(0, std::ios::end);
    _file_size = file.tellg();
    file.seekg(0);
    std::vector<std::byte> data(_file_size);
    file.read(reinterpret_cast<char*>(data.data()), data.size());
    _file_data = std::move(data);

    init();
  }

  void font_accessor::init()
  {
    auto reader = begin_read();
    // read offset subtable
    _offsets.scaler = reader.r_u32();
    _offsets.num_tables = reader.r_u16();
    _offsets.search_range = reader.r_u16();
    _offsets.entry_selector = reader.r_u16();
    _offsets.range_shift = reader.r_u16();

    seek_table(reader, font_table::tab_hhea);
    reader.s_u32(); // version
    _hhea.ascent = reader.r_u16();
    _hhea.descent = reader.r_u16();
    _hhea.line_gap = reader.r_u16();
    _hhea.advance_width_max = reader.r_u16();
    _hhea.min_left_side_bearing = reader.r_u16();
    _hhea.min_right_side_bearing = reader.r_u16();
    _hhea.x_max_extent = reader.r_u16();
    _hhea.caret_slope_rise = reader.r_u16();
    _hhea.caret_slope_run = reader.r_u16();
    _hhea.caret_offset = reader.r_u16();
    // 4x 16 bits reserved
    reader.skip(4 * sizeof(std::uint16_t));
    _hhea.metric_data_format = reader.r_u16();
    _hhea.num_long_horizontal_metrics = reader.r_u16();

    _hmtx_offset = *table_offset(font_table::tab_hmtx);

    seek_table(reader, font_table::tab_maxp);
    reader.s_u32(); // version
    _maxp.num_glyphs = reader.r_u16();
    _maxp.max_points = reader.r_u16();
    _maxp.max_contours = reader.r_u16();
    _maxp.max_composite_points = reader.r_u16();
    _maxp.max_composite_contours = reader.r_u16();

    if(font::max_expected_num_points < _maxp.max_points)
      throw std::runtime_error("There will be more points than I thought would be enough. Please increase font::max_expected_num_points.");
    if(font::max_expected_num_contours < _maxp.max_contours)
      throw std::runtime_error("There will be more contours than I thought would be enough. Please increase font::max_expected_num_contours.");


    seek_table(reader, font_table::tab_head);
    reader.s_u32(); // version
    reader.s_u32(); // revision
    _header.checksum_adjustment = reader.r_u32();
    auto const magic_number = reader.r_u32();
    if (magic_number != ttf_magic_number)
      throw std::runtime_error("Error while reading font: invalid magic number.");

    _header.flags = reader.r_u16();
    _header.units_per_em = reader.r_u16();
    reader.s_u64(); // created
    reader.s_u64(); // modified
    _header.x_min = reader.r_u16();
    _header.y_min = reader.r_u16();
    _header.x_max = reader.r_u16();
    _header.y_max = reader.r_u16();
    reader.s_u16(); // mac_style
    _header.lowest_recommended_ppem = reader.r_u16();
    _header.font_direction_hint = reader.r_u16();
    _header.index_to_loc_format = loc_format{ reader.r_u16() };
    _header.glyph_data_format = reader.r_u16();

    auto const offset = seek_table(reader, font_table::tab_cmap);
    std::uint16_t version = reader.r_u16();
    std::uint16_t num_subtables = reader.r_u16();

    for (int i = 0; i < num_subtables; ++i)
    {
      std::uint16_t platform_id = reader.r_u16();
      std::uint16_t encoding_id = reader.r_u16();
      std::uint32_t subtable_offset = reader.r_u32();

      if (platform_id == 0) // unicode
      {
        reader.seek_to(*offset + subtable_offset);
        std::uint16_t const format = reader.r_u16();
        std::uint16_t const length = reader.r_u16();
        std::uint16_t const language = reader.r_u16();

        if (format == 0)
        {
          _glyph_indexer = glyph_index_data_f0{ .offset = *offset + subtable_offset };
        }
        else if (format == 2)
        {
          glyph_index_data_f2 format2{
            .offset = *offset + subtable_offset
          };
          _glyph_indexer = format2;
        }
        else if (format == 4)
        {
          glyph_index_data_f4 format4{
            .offset = *offset + subtable_offset,
            .seg_count_x2 = reader.r_u16(),
            .search_range = reader.r_u16(),
            .entry_selector = reader.r_u16(),
            .range_shift = reader.r_u16(),
          };
          _glyph_indexer = format4;
        }
        else if (format == 6)
        {

          _glyph_indexer = glyph_index_data_f6{
            .offset = *offset + subtable_offset
          };
        }
        else
        {
          throw std::runtime_error("Detected glyph cmap format not supported.");
        }

        break;
      }
    }

    // GPOS
    if (auto const gpos_base = seek_table(reader, font_table::tab_gpos))
    {
      _gpos_off = gspec_off{};

      auto const major = reader.r_u16();
      auto const minor = reader.r_u16();

      _gpos_off->script_list = gpos_base.value() + reader.r_u16();
      _gpos_off->feature_list = gpos_base.value() + reader.r_u16();
      _gpos_off->lookup_list = gpos_base.value() + reader.r_u16();
      if (major == 1 && minor == 1)
        _gpos_off->feature_variations = gpos_base.value() + reader.r_u32();
    }

    // GSUB
    if (auto const gsub_base = seek_table(reader, font_table::tab_gsub))
    {
      _gsub_off = gspec_off{};

      auto const major = reader.r_u16();
      auto const minor = reader.r_u16();

      _gsub_off->script_list = gsub_base.value() + reader.r_u16();
      _gsub_off->feature_list = gsub_base.value() + reader.r_u16();
      _gsub_off->lookup_list = gsub_base.value() + reader.r_u16();
      if (major == 1 && minor == 1)
        _gsub_off->feature_variations = gsub_base.value() + reader.r_u32();
    }

    // GDEF
    if (auto const gdef_base = seek_table(reader, font_table::tab_gdef))
    {
      auto const major = reader.r_u16();
      auto const minor = reader.r_u16();


      _gdef_off.glyph_class_def_offset = gdef_base.value() + reader.r_u16();
      _gdef_off.attachment_def_offset = gdef_base.value() + reader.r_u16();
      _gdef_off.lig_caret_offset = gdef_base.value() + reader.r_u16();
      _gdef_off.mark_attach_class_def_offset = gdef_base.value() + reader.r_u16();

      if (_gdef_off.glyph_class_def_offset.value() == gdef_base.value())
        _gdef_off.glyph_class_def_offset = std::nullopt;
      if (_gdef_off.attachment_def_offset.value() == gdef_base.value())
        _gdef_off.attachment_def_offset = std::nullopt;
      if (_gdef_off.lig_caret_offset.value() == gdef_base.value())
        _gdef_off.lig_caret_offset = std::nullopt;
      if (_gdef_off.mark_attach_class_def_offset.value() == gdef_base.value())
        _gdef_off.mark_attach_class_def_offset = std::nullopt;

      if (major == 1 && minor >= 2)
        _gdef_off.mark_glyph_sets_def_offset = gdef_base.value() + reader.r_u16();
      if (major == 1 && minor >= 3)
        _gdef_off.item_var_store_offset = gdef_base.value() + reader.r_u32();
    }
  }

  std::optional<font_accessor::list_of_uint16_feature> font_accessor::attachment_points(glyph_id glyph) const
  {
    if (!_gdef_off.attachment_def_offset)
      return std::nullopt;

    auto reader = begin_read();
    reader.seek_to(_gdef_off.attachment_def_offset.value());
    auto const cov = reader.r_u16();
    auto const is_covered = coverage_index(
      _gdef_off.attachment_def_offset.value() + cov, glyph);

    if (!is_covered)
      return std::nullopt;

    reader.s_u16(); // count;
    auto const cov_index = *is_covered;
    reader.seek_to(reader.position() + cov_index * sizeof(std::uint16_t));
    reader.seek_to(_gdef_off.attachment_def_offset.value() + reader.r_u16());

    auto const count = reader.r_u16();
    auto const offset = reader.position();

    list_of_uint16_feature const ft{
      .offset = static_cast<std::ptrdiff_t>(offset),
      .num_elements16 = count,
      .stride = sizeof(std::uint16_t)
    };
    return ft;
  }

  std::optional<basic_glyph_class> font_accessor::basic_class_of(glyph_id glyph) const
  {
    if (!_gdef_off.glyph_class_def_offset)
      return std::nullopt;

    auto const found_class = class_of(_gdef_off.glyph_class_def_offset.value(), glyph);
    if (!found_class)
      return std::nullopt;

    return static_cast<basic_glyph_class>(static_cast<std::uint32_t>(found_class.value()));
  }

  font_accessor::horizontal_metric font_accessor::hmetric(glyph_id glyph) const
  {
    auto reader = begin_read();
    auto const glyph_id = std::uint32_t(glyph);
    if (glyph_id < _hhea.num_long_horizontal_metrics)
    {
      reader.seek_to(_hmtx_offset + 2 * sizeof(std::uint16_t) * glyph_id);
      horizontal_metric metric{
        .advance_width = reader.r_u16(),
        .left_bearing = static_cast<std::int16_t>(reader.r_u16()),
      };
      return metric;
    }
    else
    {
      horizontal_metric metric{};
      reader.seek_to(_hmtx_offset + 2 * sizeof(std::uint16_t) * (_hhea.num_long_horizontal_metrics - 1ull));
      metric.advance_width = reader.r_u16();
      reader.seek_to(_hmtx_offset + sizeof(std::uint16_t) * (2ull * _hhea.num_long_horizontal_metrics + static_cast<std::size_t>(glyph_id)));
      metric.left_bearing = reader.r_u16();
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

  std::int16_t font_accessor::ascent() const
  {
    return _hhea.ascent;
  }

  std::int16_t font_accessor::descent() const
  {
    return _hhea.descent;
  }

  std::optional<glyph_class> font_accessor::class_of(std::size_t class_table, glyph_id glyph) const
  {
    auto reader = begin_read();
    reader.seek_to(class_table);
    auto const format = reader.r_u16();
    auto const glyph_cur = std::uint16_t(glyph);

    if (format == 1)
    {
      auto const start_glyph = reader.r_u16();
      auto const num_glyphs = reader.r_u16();

      if (glyph_cur < start_glyph || glyph_cur >= start_glyph + num_glyphs)
        return std::nullopt;

      auto const index = start_glyph - glyph_cur;
      reader.seek_to(reader.position() + index * sizeof(std::uint16_t));
      auto const class_val = reader.r_u16();
      return glyph_class{ class_val };
    }
    else if (format == 2)
    {
      auto const num_ranges = reader.r_u16();
      auto const base_offset = reader.position();

      auto const map = [&](int k) {
        reader.seek_to(base_offset + k * 3ull * sizeof(std::uint16_t) + sizeof(std::uint16_t));
        return glyph_id(reader.r_u16());
      };

      auto const bound = std::ranges::lower_bound(std::ranges::views::iota(0, int(num_ranges)), glyph, &glyph_id_less, map);
      auto const found_index = *bound;

      reader.seek_to(base_offset + found_index * 3ull * sizeof(std::uint16_t));
      auto const lower = reader.r_u16();
      auto const upper = reader.r_u16();
      auto const class_val = reader.r_u16();

      if (glyph_cur < lower || glyph_cur > upper)
        return std::nullopt;

      return glyph_class{ class_val };
    }
    else
    {
      throw std::runtime_error("Invalid class format");
    }
  }

  glyph_id font_accessor::index_of(char32_t character) const
  {
    auto reader = begin_read();
    if (auto const* f0 = std::get_if<glyph_index_data_f0>(&_glyph_indexer))
    {
      if (character < 0 || character > 255)
        return glyph_id::missing;
      reader.seek_to(f0->offset + 3 * sizeof(std::uint16_t) + character * sizeof(std::uint8_t));
      return glyph_id(reader.r_u8());
    }
    else if (auto const* f2 = std::get_if<glyph_index_data_f2>(&_glyph_indexer))
    {
      auto const high_byte = (character >> 8) & 0xFF;

      reader.seek_to(f2->offset + 3 * sizeof(std::uint16_t) + high_byte * sizeof(std::uint16_t));
      auto const index_x8 = reader.r_u16();
      reader.seek_to(f2->offset + 3 * sizeof(std::uint16_t) + 256 * sizeof(std::uint16_t) + index_x8);
      auto const first = reader.r_u16();
      auto const count = reader.r_u16();
      auto const delta = static_cast<std::int16_t>(reader.r_u16());
      auto const range_offset_position = reader.position();
      auto const range_offset = reader.r_u16();

      auto const low_byte = character & 0xFF;
      if (first > low_byte && low_byte <= (first + count))
        return glyph_id::missing;

      auto const p = (first - low_byte);
      reader.seek_to(range_offset_position + range_offset);
      auto const first_index = reader.r_u16();
      if (p == 0)
        return glyph_id{ first_index };
      return glyph_id((first_index + std::uint16_t(p) + delta) % 0xFFFF);
    }
    else if (auto const* f4 = std::get_if<glyph_index_data_f4>(&_glyph_indexer))
    {
      auto const base_offset = f4->offset + 3 * sizeof(std::uint16_t);
      auto const end_points_offset = base_offset + 4 * sizeof(std::uint16_t);

      auto const map = [&](int k) -> char32_t {
        reader.seek_to(end_points_offset + k * sizeof(std::uint16_t));
        return char32_t(reader.r_u16());
      };

      auto const bound = std::ranges::lower_bound(std::ranges::views::iota(0, int(f4->seg_count_x2 / 2)), character, std::less<char32_t>{}, map);
      auto const found_index = *bound;

      auto const bound_upper = map(found_index);
      auto const u16_offset_bytes = sizeof(std::uint16_t) * found_index;
      auto const sized_seg_count = sizeof(std::uint16_t) * (f4->seg_count_x2 / 2);
      auto const start_codes_offset = end_points_offset + sizeof(std::uint16_t) * (f4->seg_count_x2 / 2ull + 1);
      reader.seek_to(start_codes_offset + u16_offset_bytes);
      auto const bound_lower = reader.r_u16();

      if (character < bound_lower || character > bound_upper)
        return glyph_id::missing;

      reader.seek_to(start_codes_offset + sized_seg_count + u16_offset_bytes);
      auto const id_delta = static_cast<std::int16_t>(reader.r_u16());
      reader.seek_to(start_codes_offset + sized_seg_count + sized_seg_count + u16_offset_bytes);
      auto const id_range_offset = reader.r_u16();

      if (id_range_offset == 0)
        return glyph_id{ (id_delta + character) % 65536 };

      reader.seek_to(start_codes_offset + sized_seg_count + sized_seg_count + u16_offset_bytes +
        id_range_offset + (std::size_t(character) - bound_lower) * sizeof(std::uint16_t));
      return glyph_id(reader.r_u16());
    }
    else if (auto const* f6 = std::get_if<glyph_index_data_f6>(&_glyph_indexer))
    {
      reader.seek_to(f6->offset + 3 * sizeof(std::uint16_t));
      auto const first_code = reader.r_u16();
      auto const entry_count = reader.r_u16();

      if (character < first_code || character >= first_code + entry_count)
        return glyph_id::missing;

      auto const index = character - first_code;
      reader.seek_to(reader.position() + index * sizeof(std::uint16_t));
      return glyph_id{ reader.r_u16() };
    }
    else
    {
      throw std::runtime_error("Detected glyph cmap format not supported.");
    }
  }

  std::optional<std::size_t> font_accessor::table_offset(font_table tag) const
  {
    auto reader = begin_read();
    reader.seek_to(sizeof(offset_subtable));
    auto const tag_less = [](font_table tag, font_table t) {
      return std::uint32_t(tag) < std::uint32_t(t);
    };
    auto const map = [&](int k) {
      reader.seek_to(sizeof(offset_subtable) + k * (4 * sizeof(std::uint32_t)));
      auto const ui = reader.r_u32();
      return font_table(ui);
    };

    auto const bound = std::ranges::lower_bound(std::ranges::views::iota(0, int(_offsets.num_tables)), tag, tag_less, map);

    if (tag == map(*bound))
    {
      reader.s_u32(); // checksum
      return reader.r_u32(); // offset
    }

    return std::nullopt;
  }

  font_accessor::offset_size font_accessor::glyph_offset_size_bytes(glyph_id glyph) const
  {
    auto reader = begin_read();
    auto const glyph_num = std::uint32_t(glyph);
    if (_header.index_to_loc_format == loc_format::short_int16)
    {
      reader.seek_to(*table_offset(font_table::tab_loca) + glyph_num * sizeof(std::uint16_t));
      offset_size result{
        .offset = std::ptrdiff_t(reader.r_u16()) * 2,
        .size = static_cast<std::size_t>(std::ptrdiff_t(reader.r_u16()) * 2 - result.offset),
      };
      return result;
    }
    else
    {
      reader.seek_to(*table_offset(font_table::tab_loca) + glyph_num * sizeof(std::uint32_t));
      offset_size result{};
      result.offset = std::ptrdiff_t(reader.r_u32());
      result.size = std::ptrdiff_t(reader.r_u32()) - result.offset;
      return result;
    }
  }

  std::optional<std::uint16_t> font_accessor::script_offset(font_script script, std::optional<gspec_off> const& offset) const
  {
    if (!offset)
      return std::nullopt;
    auto reader = begin_read();

    reader.seek_to(offset->script_list);

    std::uint16_t num_scripts = reader.r_u16();

    struct script_ref
    {
      char tag[4];
      std::int16_t script_offset;
    };

    for (int i = 0; i < num_scripts; ++i)
    {
      if (font_script{ reader.r_u32() } == script)
        return reader.r_u16();
      reader.s_u16();
    }
    return std::nullopt;
  }

  std::optional<std::uint16_t> font_accessor::lang_offset(font_language lang, std::uint16_t script, std::optional<gspec_off> const& offset) const
  {
    if (!offset)
      return std::nullopt;
    auto reader = begin_read();

    reader.seek_to(offset->script_list + script);

    auto const default_offset = reader.r_u16();
    auto const count = reader.r_u16();

    for (int i = 0; i < count; ++i)
    {
      if (font_language{ reader.r_u32() } == lang)
        return reader.r_u16();
      reader.s_u16();
    }

    if (default_offset == 0)
      return std::nullopt;
    return default_offset;
  }

  std::optional<std::uint16_t> font_accessor::feature_offset(font_feature feat, std::uint16_t lang, std::optional<gspec_off> const& offset) const
  {
    if (!offset)
      return std::nullopt;

    auto reader = begin_read();
    reader.seek_to(offset->script_list + lang);

    // search in featureIndices for feature with the given tag
    reader.s_u16(); // lookupOrderOffset
    reader.s_u16(); // required feature index
    std::uint16_t const feature_index_count = reader.r_u16();

    for (int i = 0; i < feature_index_count; ++i)
    {
      auto const index = reader.r_u16();
      auto const next = reader.position();

      reader.seek_to(offset->feature_list + sizeof(std::uint16_t) + index * (4 * sizeof(std::byte) + sizeof(std::int16_t)));
      auto const rd = font_feature{ reader.r_u32() };
      if (feat == rd)
      {
        return reader.r_u16();
      }
      reader.seek_to(next);
    }
    return std::nullopt;
  }

  std::optional<std::size_t> font_accessor::coverage_index(std::size_t offset, glyph_id glyph) const
  {
    auto reader = begin_read();
    auto const prev = reader.position();
    reader.seek_to(offset);

    auto const format = reader.r_u16();
    if (format == 1)
    {
      auto const count = reader.r_u16();
      auto const base_offset = reader.position();

      auto const map = [&](int k) {
        reader.seek_to(base_offset + k * sizeof(std::uint16_t));
        return glyph_id(reader.r_u16());
      };

      auto const bound = std::ranges::lower_bound(std::ranges::views::iota(0, int(count)), glyph, &glyph_id_less, map);

      auto const found_index = *bound;
      auto const g = map(found_index);

      reader.seek_to(prev);

      if (g == glyph)
        return found_index;
      return std::nullopt;
    }
    else if (format == 2)
    {
      auto const count = reader.r_u16();
      auto const base_offset = reader.position();

      auto const map = [&](int k) {
        reader.seek_to(base_offset + k * (sizeof(std::uint16_t) * 3) + sizeof(std::uint16_t));
        return glyph_id(reader.r_u16());
      };

      auto const bound = std::ranges::upper_bound(std::ranges::views::iota(0, int(count)), glyph, &glyph_id_less, map);
      auto const found_index = *bound;

      reader.seek_to(base_offset + found_index * (sizeof(std::uint16_t) * 3));
      auto const lower = reader.r_u16();
      auto const upper = reader.r_u16();
      auto const start_index = reader.r_u16();
      std::uint32_t const g = std::uint32_t(glyph);

      if (g < lower || g > upper)
        return std::nullopt;

      reader.seek_to(prev);

      return start_index + (g - lower);
    }
    else
    {
      throw std::runtime_error("Unknown coverage format");
    }

    reader.seek_to(prev);
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

  font_accessor::value_record font_accessor::r_value(ptr_reader& reader, std::uint16_t flags) const
  {
    value_record rec{};
    if ((flags & val_flag_x_placement) != 0)
      rec.x_placement = reader.r_u16();
    if ((flags & val_flag_y_placement) != 0)
      rec.y_placement = reader.r_u16();
    if ((flags & val_flag_x_advance) != 0)
      rec.x_advance = reader.r_u16();
    if ((flags & val_flag_y_advance) != 0)
      rec.y_advance = reader.r_u16();
    if ((flags & val_flag_x_placement_device) != 0)
      rec.x_placement_device_off = reader.r_u16();
    if ((flags & val_flag_y_placement_device) != 0)
      rec.y_placement_device_off = reader.r_u16();
    if ((flags & val_flag_x_advance_device) != 0)
      rec.x_advance_device_off = reader.r_u16();
    if ((flags & val_flag_y_advance_device) != 0)
      rec.y_advance_device_off = reader.r_u16();
    return rec;
  }

  template<typename Fun>
  auto lookup(
    font_accessor const& accessor, std::uint16_t feat, std::span<glyph_id const> glyphs, std::optional<font_accessor::gspec_off> const& offset, Fun&& func)
    -> std::invoke_result_t<Fun, font_accessor::lookup_query>
  {
    if (!offset)
      return std::nullopt;

    auto reader = accessor.begin_read();

    reader.seek_to(offset->feature_list + feat);
    reader.s_u16(); // feature params offset
    auto const count = reader.r_u16();

    for (int i = 0; i < count; ++i)
    {
      auto const index = reader.r_u16();
      auto const next = reader.position();

      reader.seek_to(offset->lookup_list + sizeof(std::uint16_t) + index * sizeof(std::int16_t));
      auto const lookup_offset = reader.r_u16();
      reader.seek_to(offset->lookup_list + lookup_offset);

      auto const lookup_type = reader.r_u16();
      auto const lookup_flag = reader.r_u16();
      auto const num_subtables = reader.r_u16();
      font_accessor::lookup_query query{};
      query.flags = lookup_flag;
      query.type = lookup_type;

      for (int sub = 0; sub < num_subtables; ++sub)
      {
        auto const subtable_offset = reader.r_u16();
        auto const next_subtable_offset = reader.position();

        reader.seek_to(offset->lookup_list + lookup_offset + subtable_offset);
        query.lookup_offset = lookup_offset;
        query.subtable_offset = subtable_offset;

        if (auto const res = std::invoke(func, query))
          return res;

        reader.seek_to(next_subtable_offset);
      }

      reader.seek_to(next);
    }

    return std::nullopt;
  }

  std::optional<font_accessor::gsub_feature> font_accessor::gsub_feature_lookup(std::uint16_t feat, std::span<glyph_id const> glyphs) const
  {
    auto reader = begin_read();
    enum class lookup_type_t
    {
      single_sub = 1,
      multi_sub = 2,
      alternate = 3,
      ligature = 4,
      context = 5,
      chaining_context = 6,
      extension_sub = 7,
      reverse_chaining_context_single = 8
    };

    return lookup(*this, feat, glyphs, _gsub_off, [&](lookup_query const& q) -> std::optional<gsub_feature> {
      reader.seek_to(_gsub_off->lookup_list + q.lookup_offset + q.subtable_offset);
      lookup_type_t const type = lookup_type_t(q.type);
      switch (type)
      {
      case lookup_type_t::single_sub:
      {
        auto const format = reader.r_u16();
        auto const coverage_offset = reader.r_u16();
        auto const covered = coverage_index(_gsub_off->lookup_list + q.lookup_offset + q.subtable_offset + coverage_offset, glyphs[0]);

        if (!covered)
          break;

        if (format == 1)
        {
          std::int16_t const offset = reader.r_u16();
          std::uint16_t const gly = std::uint16_t(glyphs[0]);
          return mono_substitution_feature{ .substitution = glyph_id(gly + offset) };
        }
        else if (format == 2)
        {
          auto const count = reader.r_u16();
          reader.seek_to(reader.position() + covered.value() * sizeof(std::uint16_t));
          return mono_substitution_feature{ .substitution = glyph_id(reader.r_u16()) };
        }
        else
        {
          throw std::runtime_error("Not implemented");
        }
      }
      break;
      case lookup_type_t::multi_sub:
      {
        auto const format = reader.r_u16();
        auto const coverage_offset = reader.r_u16();
        auto const covered = coverage_index(_gsub_off->lookup_list + q.lookup_offset + q.subtable_offset + coverage_offset, glyphs[0]);

        if (!covered)
          break;

        auto const seq_count = reader.r_u16();
        reader.seek_to(reader.position() + covered.value() * sizeof(std::uint16_t));
        auto const seq_offset = reader.r_u16();
        reader.seek_to(_gsub_off->lookup_list + q.lookup_offset + q.subtable_offset + seq_offset);
        auto const glyph_count = reader.r_u16();
        auto const offset = reader.position();
        auto constexpr stride = sizeof(std::uint16_t);
        return list_of_uint16_feature{
          .offset = std::ptrdiff_t(offset),
          .num_elements16 = glyph_count,
          .stride = stride
        };
      }
      break;
      case lookup_type_t::alternate:
      {
        auto const format = reader.r_u16();
        auto const coverage_offset = reader.r_u16();
        auto const covered = coverage_index(_gsub_off->lookup_list + q.lookup_offset + q.subtable_offset + coverage_offset, glyphs[0]);

        if (!covered)
          break;

        auto const set_count = reader.r_u16();
        reader.seek_to(reader.position() + covered.value() * sizeof(std::uint16_t));
        auto const set_offset = reader.r_u16();
        reader.seek_to(_gsub_off->lookup_list + q.lookup_offset + q.subtable_offset + set_offset);
        auto const glyph_count = reader.r_u16();
        auto const offset = reader.position();
        auto constexpr stride = sizeof(std::uint16_t);
        return list_of_uint16_feature{
          .offset = std::ptrdiff_t(offset),
          .num_elements16 = glyph_count,
          .stride = stride
        };
      }
      break;
      case lookup_type_t::ligature:
      {
        auto const format = reader.r_u16();
        auto const coverage_offset = reader.r_u16();
        auto const covered = coverage_index(_gsub_off->lookup_list + q.lookup_offset + q.subtable_offset + coverage_offset, glyphs[0]);

        if (!covered)
          break;

        auto const lig_set_count = reader.r_u16();
        reader.seek_to(reader.position() + *covered * sizeof(std::uint16_t));
        auto const lig_set_offset = reader.r_u16();

        reader.seek_to(_gsub_off->lookup_list + q.lookup_offset + q.subtable_offset + lig_set_offset);
        auto const lig_count = reader.r_u16();
        auto const base_offset = _gsub_off->lookup_list + q.lookup_offset + q.subtable_offset + lig_set_offset;

        for (int i = 0; i < lig_count; ++i)
        {
          reader.seek_to(base_offset + sizeof(std::uint16_t) + i * sizeof(std::uint16_t));
          auto const offset = reader.r_u16();
          reader.seek_to(base_offset + offset);

          mono_substitution_feature feat{ .substitution = glyph_id(reader.r_u16()) };

          bool matches = true;
          auto const num_comps = reader.r_u16();
          if (num_comps != glyphs.size())
            continue;

          for (int c = 1; c < num_comps; ++c)
          {
            bool const glyph_ok = glyph_id(reader.r_u16()) == glyphs[c];
            if (!glyph_ok)
            {
              matches = false;
              break;
            }
          }

          if (matches)
            return feat;
        }
      }
      break;
      case lookup_type_t::context:
      case lookup_type_t::chaining_context:
      case lookup_type_t::extension_sub:
      case lookup_type_t::reverse_chaining_context_single:
      default:
        throw std::runtime_error("Not implemented");
        break;
      }
      return std::nullopt;
      });
  }

  std::optional<font_accessor::gpos_feature> font_accessor::gpos_feature_lookup(std::uint16_t feat, std::span<glyph_id const> glyphs) const
  {
    auto reader = begin_read();
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

    return lookup(*this, feat, glyphs, _gpos_off, [&](lookup_query const& q) -> std::optional<gpos_feature> {
      reader.seek_to(_gpos_off->lookup_list + q.lookup_offset + q.subtable_offset);
      auto const lookup_type = lookup_type_t{ q.type };
      switch (lookup_type)
      {
      case lookup_type_t::single_adj:
      {
        if (glyphs.size() != 1)
        {
          // we did not ask for a one-glyph-thing...
          break;
        }

        auto const fmt = reader.r_u16();
        auto const coverage_offset = reader.r_u16();
        auto const glyph = std::uint32_t(glyphs[0]);

        auto covered = coverage_index(_gpos_off->lookup_list + q.lookup_offset + q.subtable_offset + coverage_offset, glyphs[0]);

        if (covered)
        {
          auto const value_format = reader.r_u16();

          if (fmt == 1)
          {
            // All have the same value format
            return r_value(reader, value_format);
          }
          else
          {
            // We have one item per covered glyph
            auto const rec_size = value_record_size(value_format);
            reader.s_u16(); // value_count
            reader.seek_to(reader.position() + rec_size * *covered);
            return r_value(reader, value_format);
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

        auto const base_off = reader.position();
        auto const fmt = reader.r_u16();
        std::int16_t const coverage_offset = reader.r_u16();

        auto covered = coverage_index(_gpos_off->lookup_list + q.lookup_offset + q.subtable_offset + coverage_offset, glyphs[0]);

        if (!covered)
          break; // break out of switch (not of loop)

        auto const value_format_1 = reader.r_u16();
        auto const value_format_2 = reader.r_u16();
        auto const record_size_1 = value_record_size(value_format_1);
        auto const record_size_2 = value_record_size(value_format_2);

        if (fmt == 1)
        {
          auto const num_pair_sets = reader.r_u16();
          reader.seek_to(reader.position() + covered.value() * sizeof(std::uint16_t));
          auto const pair_set_offset = reader.r_u16();
          reader.seek_to(_gpos_off->lookup_list + q.lookup_offset + q.subtable_offset + pair_set_offset);
          auto const pair_value_count = reader.r_u16();
          auto const base_offset = reader.position();

          auto const pair_value_size = sizeof(std::uint16_t) +
            record_size_1 + record_size_2;

          auto const map = [&](int k) {
            reader.seek_to(base_offset + k * pair_value_size);
            return glyph_id(reader.r_u16());
          };

          auto const bound = std::ranges::lower_bound(std::ranges::views::iota(0, int(pair_value_count)), glyphs[1], &glyph_id_less, map);
          auto const found_index = *bound;
          auto const mp = map(found_index - 1);
          reader.seek_to(base_offset + found_index * pair_value_size);
          auto const other_glyph = reader.r_u16(); // glyph
          if (std::uint32_t(other_glyph) != std::uint32_t(glyphs[1]))
            break; // not covered

          auto const value_1 = r_value(reader, value_format_1);
          auto const value_2 = r_value(reader, value_format_2);

          return pair_value_feature(value_1, value_2);
        }
        else if (fmt == 2)
        {
          auto const class_def1_off = reader.r_u16();
          auto const class_def2_off = reader.r_u16();
          auto const class_def1_count = reader.r_u16();
          auto const class_def2_count = reader.r_u16();
          auto const base_of_class1_recs = reader.position();

          auto const class1 = class_of(_gpos_off->lookup_list + q.lookup_offset + q.subtable_offset + class_def1_off, glyphs[0]);
          auto const class2 = class_of(_gpos_off->lookup_list + q.lookup_offset + q.subtable_offset + class_def2_off, glyphs[1]);

          if (!class1 || !class2)
            break; // not all are classed.

          auto const class1_int = std::uint16_t(class1.value());
          auto const class2_int = std::uint16_t(class2.value());

          auto const class2_rec_size = record_size_1 + record_size_2;
          auto const record = class_def2_count * class2_rec_size * class1_int + class2_rec_size * class2_int;

          reader.seek_to(base_of_class1_recs + record);

          auto const value_1 = r_value(reader, value_format_1);
          auto const value_2 = r_value(reader, value_format_2);
          return pair_value_feature(value_1, value_2);
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
      return std::nullopt;
      });
  }

  std::optional<std::size_t> font_accessor::seek_table(ptr_reader& reader, font_table tag) const
  {
    auto const offset = table_offset(tag);
    if (offset) reader.seek_to(offset.value());
    return offset;
  }

  font_feature_info::font_feature_info(font_feature_type type, std::uint16_t offset)
    : _offset(offset), _type(type)
  {

  }

  std::uint16_t font_feature_info::offset() const
  {
    return _offset;
  }

  font_feature_type font_feature_info::type() const
  {
    return _type;
  }

  glyph_id font::glyph(char32_t character) const
  {
    return _accessor.index_of(character);
  }

  std::size_t font::num_glyphs() const
  {
    return _accessor.num_glyphs();
  }

  std::pair<float, float> font::advance_bearing(glyph_id current) const
  {
    auto hmetric = _accessor.hmetric(current);

    auto const rec = get_rect(current);
    if (hmetric.advance_width == 0)
    {
      hmetric.advance_width = rec.size.x;
      hmetric.left_bearing = rec.position.x;
    }
    auto const bearing = rec.position.x - hmetric.left_bearing;
    auto const advance = hmetric.advance_width;

    return { float(advance), float(bearing) };
  }

  font::font(std::filesystem::path const& path)
    : _accessor{ path }
  {
  }

  font::font(std::span<std::byte const> data, bool copy)
    : _accessor{ data, copy }
  {

  }

  std::optional<font_feature_info> font::query_feature(font_feature_type type, font_language language, font_feature feature) const
  {
    return query_feature(type, font_script::scr_default, language, feature);
  }

  std::optional<font_feature_info> font::query_feature(font_feature_type type, font_feature feature) const
  {
    return query_feature(type, font_language::lang_default, feature);
  }

  std::optional<font_feature_info> font::query_feature(font_feature_type type, font_script script, font_language language, font_feature feature) const
  {
    auto const offset = [&]() -> std::optional<font_accessor::gspec_off> {
      switch (type)
      {
      case font_feature_type::positioning: return _accessor.gpos();
      case font_feature_type::substitution: return _accessor.gsub();
      default:
        return std::nullopt;
      }
    }();

    if (!offset)
      return std::nullopt;

    auto script_offset = _accessor.script_offset(script, offset);
    if (!script_offset)
      return std::nullopt;

    auto const lang_offset = _accessor.lang_offset(language, *script_offset, offset);
    if (!lang_offset)
      return std::nullopt;

    auto const feature_offset = _accessor.feature_offset(feature, *script_offset + *lang_offset, offset);
    if (!feature_offset)
      return std::nullopt;

    return font_feature_info(type, *feature_offset);
  }

  std::optional<font::positioning_feature> font::lookup_positioning(font_feature_info const& info, std::span<glyph_id const> glyphs) const
  {
    if (info.type() != font_feature_type::positioning)
      throw std::invalid_argument("Provided feature must be a positioning feature");

    return _accessor.gpos_feature_lookup(info.offset(), glyphs);
  }

  std::optional<font::substitution_feature> font::lookup_substitution(font_feature_info const& info, std::span<glyph_id const> glyphs) const
  {
    if (info.type() != font_feature_type::substitution)
      throw std::invalid_argument("Provided feature must be a substitution feature");

    return _accessor.gsub_feature_lookup(info.offset(), glyphs);
  }

  size_t font::substitution_count(font::substitution_feature const& feature) const
  {
    struct
    {
      size_t operator()(font_accessor::mono_substitution_feature const& sub) const
      {
        return 1;
      }

      size_t operator()(font_accessor::list_of_uint16_feature const& sub) const
      {
        return sub.num_elements16;
      }

    } get_glyph_count;

    return std::visit(get_glyph_count, feature);
  }

  glyph_id font::substitution_glyph(font::substitution_feature const& feature, std::size_t index) const
  {
    struct
    {
      glyph_id operator()(font_accessor::mono_substitution_feature const& sub) const
      {
        return sub.substitution;
      }

      glyph_id operator()(font_accessor::list_of_uint16_feature const& sub)
      {
        if (i >= sub.num_elements16)
          return glyph_id{ 0 };
        reader.seek_to(sub.offset + i * sub.stride);
        return glyph_id(reader.r_u16());
      }

      size_t i;
      ptr_reader reader;
    } get_glyph{ index, _accessor.begin_read() };

    return std::visit(get_glyph, feature);
  }

  float goop::font::units_per_em() const
  {
    return _accessor.units_per_em();
  }

  float font::ascent() const
  {
    return _accessor.ascent();
  }

  float font::descent() const
  {
    return _accessor.descent();
  }

  rect font::get_rect(glyph_id glyph) const
  {
    auto const os = _accessor.glyph_offset_size_bytes(glyph);
    if (os.size == 0)
      return { {0,0},{0,0} };

    auto reader = _accessor.begin_read();
    reader.seek_to(*_accessor.table_offset(font_table::tab_glyf) + os.offset);

    std::int16_t const num_contours = reader.r_u16();
    std::int16_t const x_min = reader.r_u16();
    std::int16_t const y_min = reader.r_u16();
    std::int16_t const x_max = reader.r_u16();
    std::int16_t const y_max = reader.r_u16();
    rect bounds;
    bounds.position.x = x_min;
    bounds.position.y = y_min;
    bounds.size.x = x_max - x_min;
    bounds.size.y = y_max - y_min;
    return bounds;
  }

  template<typename T, std::size_t Size>
  struct stack_buffer
  {
  public:
    template<typename... Args>
    T& emplace_back(Args&&... args)
    {
      auto& val = *new(&_buffer[_put++]) T(std::forward<Args>(args)...);
      assert(_put <= Size);
      return val;
    }

    void push_back(T const& value)
    {
      new(&_buffer[_put++]) T{ value };
      assert(_put <= Size);
    }

    void push_back(T&& value)
    {
      new(&_buffer[_put++]) T{ std::move(value) };
      assert(_put <= Size);
    }

    std::size_t size() const
    {
      return _put;
    }

    T const& operator[](std::size_t index) const
    {
      return _buffer[index];
    }

    T& operator[](std::size_t index)
    {
      return _buffer[index];
    }

    std::span<T> view()
    {
      return std::span(data(), size());
    }

    std::span<T const> view() const
    {
      return std::span(data(), size());
    }

    T const* data() const
    {
      return _buffer.data();
    }

    T* data()
    {
      return _buffer.data();
    }

    auto begin() const {
      return _buffer.data();
    }

    auto end() const {
      return std::next(data(), _put);
    }

  private:

    std::size_t _put = 0;
    std::array<T, Size> _buffer{};
  };

  void font::outline_impl(glyph_id glyph, contour_buffer& contours, end_point_buffer& end_points, rect* bounds) const
  {
    auto const os = _accessor.glyph_offset_size_bytes(glyph);
    if (os.size == 0)
    {
      if (bounds)
        *bounds = { {0,0},{0,0} };
      return;
    }

    auto reader = _accessor.begin_read();
    reader.seek_to(*_accessor.table_offset(font_table::tab_glyf) + os.offset);

    std::int16_t const num_contours = reader.r_u16();
    std::int16_t const x_min = reader.r_u16();
    std::int16_t const y_min = reader.r_u16();
    std::int16_t const x_max = reader.r_u16();
    std::int16_t const y_max = reader.r_u16();

    if (bounds)
    {
      bounds->position.x = x_min;
      bounds->position.y = y_min;
      bounds->size.x = x_max - x_min;
      bounds->size.y = y_max - y_min;
    }

    auto const base_end_point_count = end_points.size();
    auto const base_end_point = contours.size();
    if (num_contours >= 0)
    {
      // simple glyph
      for (int i = 0; i < num_contours; ++i)
        end_points.push_back(base_end_point + reader.r_u16());
      std::uint16_t const instruction_length = reader.r_u16();
      reader.skip(instruction_length * sizeof(std::uint8_t));

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
          next.flags = reader.r_u8();
          next.on_line = next.flags & flag_on_curve;
          if (next.flags & flag_repeat)
          {
            auto const times = reader.r_u8();
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
          auto const x = reader.r_u8();
          c.pos.x = sign * std::int16_t(x);
        }
        else if (c.flags & flag_x_is_same_or_positive_x_short_vector)
        {
          c.pos.x = 0;
        }
        else
        {
          c.pos.x = static_cast<std::int16_t>(reader.r_u16());
        }
      }

      std::int16_t prev_y = 0;
      for (int m = start_contour; m < contours.size(); ++m)
      {
        auto& c = contours[m];
        if (c.flags & flag_y_short)
        {
          auto const sign = ((c.flags & flag_y_is_same_or_positive_y_short_vector) != 0) ? 1 : -1;
          auto const y = reader.r_u8();
          c.pos.y = sign * std::int16_t(y);
        }
        else if (c.flags & flag_y_is_same_or_positive_y_short_vector)
        {
          c.pos.y = 0;
        }
        else
        {
          c.pos.y = static_cast<std::int16_t>(reader.r_u16());
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
        flags = reader.r_u16();
        auto const index_of_component = reader.r_u16();
        auto const arg1 = (flags & flag_arg_1_and_2_are_words) ? reader.r_u16() : reader.r_u8();
        auto const arg2 = (flags & flag_arg_1_and_2_are_words) ? reader.r_u16() : reader.r_u8();

        std::int16_t a = 1, b = 0, c = 0, d = 1;

        if (flags & flag_we_have_scale)
        {
          a = reader.r_u16();
          d = a;
        }
        else if (flags & flag_we_have_x_and_y_scale)
        {
          a = reader.r_u16();
          d = reader.r_u16();
        }
        else if (flags & flag_we_have_a_two_by_two)
        {
          a = reader.r_u16();
          b = reader.r_u16();
          c = reader.r_u16();
          d = reader.r_u16();
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

        size_t const pos = reader.position();

        auto const shape_begin = contours.size();
        rect unused_rect;
        outline_impl(glyph_id{ index_of_component }, contours, end_points, &unused_rect);
        auto const shape_end = contours.size();

        auto const shape_points = contours.view().subspan(shape_begin, shape_end - shape_begin);
        auto const base_points = contours.view().subspan(0, base_shape_size);

        float e = 0;
        float f = 0;
        if (flags & flag_args_are_xy_values)
        {
          e = arg1;
          f = arg2;
        }
        else
        {
          auto const& pt1 = shape_points[arg1];
          auto const& pt2 = base_points[arg2];
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

        reader.seek_to(pos);
      }
    }
  }

  void font::outline_impl(glyph_id glyph, rect& bounds, void(*emit_segment)(void*, outline_segment), void* user_ptr) const
  {
    contour_buffer prealloc_contour_buffer;
    end_point_buffer end_points;
    outline_impl(glyph, prealloc_contour_buffer, end_points, &bounds);

    {
      int start_point = 0;
      for (auto const end_point : end_points)
      {
        auto points = prealloc_contour_buffer.view().subspan(start_point, end_point + 1ull - start_point);
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
            return decltype(prev_point.pos)((prev_point.pos + this_point.pos) / 2);
        };

        bool first = true;
        bool last_was_on_line = false;
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
              emit_segment(user_ptr, line{
                .start = last_control_point,
                .end = this_control_point
                });
            }
            else
            {
              emit_segment(user_ptr, bezier{
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