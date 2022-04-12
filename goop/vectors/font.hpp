#pragma once

#include "font_tag.hpp"
#include "font_scripts.hpp"
#include "font_languages.hpp"
#include "font_features.hpp"
#include <filesystem>
#include <unordered_map>
#include <memory>
#include "lines.hpp"
#include <span>
#include <any>
#include <fstream>
#include <sstream>
#include <functional>

namespace goop
{
  enum class glyph_id : std::uint32_t {};
  enum class glyph_class : std::uint16_t {};

  class font_accessor
  {
  public:
    struct lookup_query
    {
      std::uint16_t type;
      std::uint16_t flags;
      std::size_t lookup_offset;
      std::size_t subtable_offset;
    };

    struct gspec_off
    {
      std::size_t script_list = 0;
      std::size_t feature_list = 0;
      std::size_t lookup_list = 0;
      std::size_t feature_variations = 0;
    };

    struct horizontal_metric
    {
      std::uint16_t advance_width;
      std::int16_t left_bearing;
    };

    struct offset_size
    {
      std::ptrdiff_t offset;
      std::size_t size;
    };

    struct value_record
    {
      std::int16_t x_placement;
      std::int16_t y_placement;
      std::int16_t x_advance;
      std::int16_t y_advance;

      std::int16_t x_placement_device_off;
      std::int16_t y_placement_device_off;
      std::int16_t x_advance_device_off;
      std::int16_t y_advance_device_off;
    };

    struct mono_substitution_feature
    {
      glyph_id substitution;
    };

    using mono_value_feature = value_record;
    using pair_value_feature = std::pair<value_record, value_record>;

    using gpos_feature = std::variant<mono_value_feature, pair_value_feature>;
    using gsub_feature = std::variant<mono_substitution_feature>;

    font_accessor(std::filesystem::path const& path);

    std::optional<std::size_t> table_offset(font_tag tag);
    std::optional<std::size_t> seek_table(font_tag tag);
    void seek_to(std::size_t offset);
    std::size_t position();
    horizontal_metric hmetric(glyph_id glyph);
    offset_size glyph_offset_size_bytes(glyph_id glyph);
    std::optional<glyph_id> index_of(char32_t character);
    std::optional<glyph_class> class_of(std::size_t class_table, glyph_id glyph);
    std::size_t units_per_em() const;

    // GPOS
    std::optional<std::uint16_t> script_offset(font_script script, std::optional<gspec_off> const& offset);
    std::optional<std::uint16_t> lang_offset(font_language lang, std::uint16_t script, std::optional<gspec_off> const& offset);
    std::optional<std::uint16_t> feature_offset(font_feature feat, std::uint16_t lang, std::optional<gspec_off> const& offset);
    std::optional<gpos_feature> gpos_feature_lookup(std::uint16_t feat, std::span<glyph_id const> glyphs);
    std::optional<gsub_feature> gsub_feature_lookup(std::uint16_t feat, std::span<glyph_id const> glyphs);


    // read funcs
    std::uint8_t r_u8();
    std::uint16_t r_u16();
    std::uint32_t r_u32();
    std::uint64_t r_u64();
    font_tag r_tag();

    // skip funcs
    void s_u8();
    void s_u16();
    void s_u32();
    void s_u64();
    void skip(std::size_t bytes);

    std::optional<gspec_off> const& gpos() const;
    std::optional<gspec_off> const& gsub() const;
    bool good() const { return _file.good(); }

  private:
    std::optional<std::size_t> coverage_index(std::size_t offset, glyph_id glyph);
    value_record r_value(std::uint16_t flags);

    static constexpr size_t table_size = 16;
    static constexpr size_t ttf_magic_number = 0x5F0F3CF5;

    enum class loc_format : uint16_t
    {
      short_int16 = 0,
      long_int32 = 1
    };

    struct glyph_index_data_f0 {
      size_t offset;
    };
    struct glyph_index_data_f4 
    { 
      size_t offset;
      std::uint16_t seg_count_x2;
      std::uint16_t search_range;
      std::uint16_t entry_selector;
      std::uint16_t range_shift;
    };
    using glyph_index_data = std::variant<glyph_index_data_f0, glyph_index_data_f4>;

    struct offset_subtable
    {
      uint32_t scaler;
      uint16_t num_tables;
      uint16_t search_range;
      uint16_t entry_selector;
      uint16_t range_shift;
    } _offsets;

    struct header
    {
      std::uint32_t checksum_adjustment;
      std::uint16_t flags;
      std::uint16_t units_per_em;
      std::int16_t x_min;
      std::int16_t y_min;
      std::int16_t x_max;
      std::int16_t y_max;
      std::uint16_t lowest_recommended_ppem;
      std::int16_t font_direction_hint;
      loc_format index_to_loc_format;
      std::int16_t glyph_data_format;
    } _header;

    struct hhea
    {
      std::int16_t ascent;
      std::int16_t descent;
      std::int16_t line_gap;
      std::uint16_t advance_width_max;
      std::int16_t min_left_side_bearing;
      std::int16_t min_right_side_bearing;
      std::int16_t x_max_extent;
      std::int16_t caret_slope_rise;
      std::int16_t caret_slope_run;
      std::int16_t caret_offset;
      std::int16_t metric_data_format;
      std::int16_t num_long_horizontal_metrics;
    } _hhea;

    glyph_index_data _glyph_indexer;
    std::optional<gspec_off>  _gpos_off;
    std::optional<gspec_off>  _gsub_off;
    std::size_t _file_size;
    std::size_t _hmtx_offset;
    std::uint16_t _num_glyphs;
    std::ifstream _file;
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

    void outline(glyph_id glyph, std::vector<lines::line_segment>& segments, rect& bounds) const;

    float units_per_em() const;

    std::pair<float, float> advance_bearing(glyph_id current, glyph_id next = glyph_id{0}) const;
    std::optional<glyph_id> try_substitution(std::span<glyph_id const> glyphs) const;
    rect get_rect(glyph_id glyph) const;

  private:
    struct contour_point
    {
      std::uint8_t flags;
      bool on_line;
      rnu::vec2 pos;
    };
    void outline_impl(glyph_id glyph, std::vector<font_file::contour_point>& contours, std::vector<std::uint16_t>& end_points, rect* bounds) const;
    void try_load_gpos();
    void try_load_gsub();
    std::optional<font_accessor::pair_value_feature> lookup_kerning(glyph_id first, glyph_id second) const;

    struct gpos_features
    {
      std::optional<std::uint16_t> kerning;
    } _gpos_features;

    struct gsub_features
    {
      std::optional<std::uint16_t> ligature;
    } _gsub_features;

    mutable font_accessor _accessor;
  };
}