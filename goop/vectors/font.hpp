#pragma once

#include "font_table.hpp"
#include "font_scripts.hpp"
#include "font_languages.hpp"
#include "font_features.hpp"

#include <rnu/math/math.hpp>

#include <filesystem>
#include <span>
#include <any>
#include <fstream>
#include <variant>
#include <vector>

namespace goop
{
  enum class glyph_id : std::uint32_t {
    missing = 0
  };

  enum class glyph_class : std::uint16_t {};

  enum class basic_glyph_class
  {
    none = 0,
    base = 1,
    ligature = 2,
    mark = 3,
    component = 4
  };

  class ptr_reader;

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

    struct list_of_uint16_feature
    {
      std::ptrdiff_t offset;
      std::size_t num_elements16;
      std::size_t stride;
    };

    using mono_value_feature = value_record;
    using pair_value_feature = std::pair<value_record, value_record>;

    using gpos_feature = std::variant<mono_value_feature, pair_value_feature>;
    using gsub_feature = std::variant<mono_substitution_feature, list_of_uint16_feature>;

    font_accessor(std::filesystem::path const& path);
    font_accessor(std::span<std::byte const> data, bool copy = false);

    std::optional<std::size_t> table_offset(font_table tag) const;
    std::optional<std::size_t> seek_table(ptr_reader& reader, font_table tag) const;
    ptr_reader begin_read() const;
    horizontal_metric hmetric(glyph_id glyph) const;
    offset_size glyph_offset_size_bytes(glyph_id glyph) const;
    glyph_id index_of(char32_t character) const;
    std::optional<glyph_class> class_of(std::size_t class_table, glyph_id glyph) const;
    std::optional<basic_glyph_class> basic_class_of(glyph_id glyph) const;
    std::optional<list_of_uint16_feature> attachment_points(glyph_id glyph) const;
    std::size_t units_per_em() const;
    std::int16_t ascent() const;
    std::int16_t descent() const;

    // GPOS/GSUB
    std::optional<std::uint16_t> script_offset(font_script script, std::optional<gspec_off> const& offset) const;
    std::optional<std::uint16_t> lang_offset(font_language lang, std::uint16_t script, std::optional<gspec_off> const& offset) const;
    std::optional<std::uint16_t> feature_offset(font_feature feat, std::uint16_t lang, std::optional<gspec_off> const& offset) const;
    std::optional<gpos_feature> gpos_feature_lookup(std::uint16_t feat, std::span<glyph_id const> glyphs) const;
    std::optional<gsub_feature> gsub_feature_lookup(std::uint16_t feat, std::span<glyph_id const> glyphs) const;

    std::optional<gspec_off> const& gpos() const;
    std::optional<gspec_off> const& gsub() const;
    std::size_t num_glyphs() const;

  private:
    void init();
    std::optional<std::size_t> coverage_index(std::size_t offset, glyph_id glyph) const;
    value_record r_value(ptr_reader& reader, std::uint16_t flags) const;

    static constexpr size_t table_size = 16;
    static constexpr size_t ttf_magic_number = 0x5F0F3CF5;

    using data_variant = std::variant<std::vector<std::byte>, std::span<std::byte const>>;

    enum class loc_format : uint16_t
    {
      short_int16 = 0,
      long_int32 = 1
    };

    struct glyph_index_data_f0 {
      size_t offset;
    };
    struct glyph_index_data_f2
    {
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
    struct glyph_index_data_f6
    {
      size_t offset;
    };
    using glyph_index_data = std::variant<
      glyph_index_data_f0, 
      glyph_index_data_f2,
      glyph_index_data_f4,
      glyph_index_data_f6>;

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

    struct
    {
      std::optional<std::size_t> glyph_class_def_offset;
      std::optional<std::size_t> attachment_def_offset;
      std::optional<std::size_t> lig_caret_offset;
      std::optional<std::size_t> mark_attach_class_def_offset;
      std::optional<std::size_t> mark_glyph_sets_def_offset;
      std::optional<std::size_t> item_var_store_offset;
    } _gdef_off;

    struct
    {
      std::uint16_t num_glyphs;
      std::uint16_t max_points;
      std::uint16_t max_contours;
      std::uint16_t max_composite_points;
      std::uint16_t max_composite_contours;
    } _maxp;

    glyph_index_data _glyph_indexer;
    std::optional<gspec_off>  _gpos_off;
    std::optional<gspec_off>  _gsub_off;
    std::size_t _file_size;
    std::size_t _hmtx_offset;
    data_variant _file_data;
  };

  struct rect
  {
    rnu::vec2 position;
    rnu::vec2 size;
  };

  struct line
  {
    rnu::vec2 start;
    rnu::vec2 end;
  };

  struct bezier
  {
    rnu::vec2 start;
    rnu::vec2 control;
    rnu::vec2 end;
  };

  using outline_segment = std::variant<line, bezier>;

  enum class font_feature_type
  {
    positioning,
    substitution
  };
    
  class font_feature_info
  {
  public:
    friend class font;
    font_feature_info(font_feature_info const&) = default;
    font_feature_info(font_feature_info&&) noexcept = default;
    font_feature_info& operator=(font_feature_info const&) = default;
    font_feature_info& operator=(font_feature_info&&) noexcept = default;

  private:
    font_feature_info(font_feature_type type, std::uint16_t offset);

    std::uint16_t offset() const;
    font_feature_type type() const;

    font_feature_type _type;
    std::uint16_t _offset;
  };

  template<typename T, std::size_t Size>
  struct stack_buffer;

  class font
  {
  public:
    using positioning_feature = font_accessor::gpos_feature;
    using substitution_feature = font_accessor::gsub_feature;
    constexpr static std::size_t max_expected_num_points = 512;
    constexpr static std::size_t max_expected_num_contours = 64;

    font(std::filesystem::path const& path);
    font(std::span<std::byte const> data, bool copy = false);

    glyph_id glyph(char32_t character) const;
    std::size_t num_glyphs() const;
    
    template<typename Func>
    void outline(glyph_id glyph, rect& bounds, Func&& func) const
    {
      outline_impl(glyph, bounds, [](void* u, outline_segment s) {
        std::invoke(*static_cast<std::decay_t<Func>*>(u), s); 
      }, &func);
    }

    float units_per_em() const;
    float ascent() const;
    float descent() const;
    std::pair<float, float> advance_bearing(glyph_id current) const;

    rect get_rect(glyph_id glyph) const;

    std::optional<font_feature_info> query_feature(font_feature_type type, font_script script, font_language language, font_feature feature) const;
    std::optional<font_feature_info> query_feature(font_feature_type type, font_language language, font_feature feature) const;
    std::optional<font_feature_info> query_feature(font_feature_type type, font_feature feature) const;

    std::optional<positioning_feature> lookup_positioning(font_feature_info const& info, std::span<glyph_id const> glyphs) const;
    std::optional<substitution_feature> lookup_substitution(font_feature_info const& info, std::span<glyph_id const> glyphs) const;

    size_t substitution_count(substitution_feature const& feature) const;
    glyph_id substitution_glyph(substitution_feature const& feature, std::size_t index) const;

  private:
    struct contour_point
    {
      std::uint8_t flags = 0;
      std::uint8_t on_line = false;
      rnu::vec2 pos;
    };

    using contour_buffer = stack_buffer<contour_point, max_expected_num_points>;
    using end_point_buffer = stack_buffer<std::uint16_t, max_expected_num_contours>;

    void outline_impl(glyph_id glyph, rect& bounds, void(*emit_segment)(void*, outline_segment), void* user_ptr) const;
    void outline_impl(glyph_id glyph, contour_buffer& contours, end_point_buffer& end_points, rect* bounds) const;
    
    font_accessor _accessor;
  };
}