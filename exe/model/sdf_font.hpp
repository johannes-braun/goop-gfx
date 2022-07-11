#pragma once

#include <vectors/font.hpp>
#include <vectors/lines.hpp>
#include <generic/handle.hpp>
#include <span>
#include <graphics.hpp>
#include <vector>
#include <unordered_map>
#include <optional>
#include <rnu/math/math.hpp>

namespace goop::gui
{
  class sdf_font_base
  {
  public:
    friend class sdf_font;
    struct set_glyph_t
    {
      glyph_id glyph;
      rnu::rect2f bounds;
      rnu::rect2f uvs;
    };

    goop::texture const& atlas_texture();
    std::vector<set_glyph_t> text_set(std::wstring_view str, int* num_lines = nullptr, float* x_max = nullptr);

    float base_size() const;
    float sdf_width() const;
    float em_factor(float target_size) const;
    float line_height() const;
    font const& font() const;

  private:
    void load(int atlas_width, float base_size, float sdf_width, goop::font font, std::span<std::pair<char16_t, char16_t> const> unicode_ranges);
    void dump(std::vector<std::uint8_t>& image, int& w, int& h) const;
    std::vector<goop::lines::line> const& load_glyph(glyph_id glyph) const;
    std::optional<glyph_id> ligature(goop::font const& font, goop::font_feature_info const& lig_feature, std::span<goop::glyph_id const> glyphs);

    struct glyph_info
    {
      char16_t character;
      goop::glyph_id id;
      float scale;
      rnu::rect2f default_bounds;
      rnu::rect2f packed_bounds;

      rnu::rect2f error;
    };

    std::optional<goop::font> _font;
    float _base_size = 0;
    float _sdf_width = 0;
    int _width = 0;
    int _height = 0;
    std::unordered_map<glyph_id, glyph_info> _infos;

    std::optional<goop::texture> _texture;
  };

  class sdf_font : public goop::handle<sdf_font_base, sdf_font_base> 
  {
  public:
    sdf_font(int atlas_width, float base_size, float sdf_width, goop::font font, std::span<std::pair<char16_t, char16_t> const> unicode_ranges);
  };
}