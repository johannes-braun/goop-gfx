#include "sdf_font.hpp"
#include <vectors/skyline_packer.hpp>
#include <algorithm>
#include <execution>

namespace goop::gui
{
  sdf_font::sdf_font(int atlas_width, float base_size, float sdf_width, goop::font font, std::span<std::pair<char16_t, char16_t> const> unicode_ranges)
  {
    (*this)->load(atlas_width, base_size, sdf_width, std::move(font), std::move(unicode_ranges));
  }
  void sdf_font_base::load(int atlas_width, float base_size, float sdf_width, goop::font font, std::span<std::pair<char16_t, char16_t> const> unicode_ranges)
  {
    _font = std::move(font);
    _base_size = base_size;
    _sdf_width = sdf_width;
    _width = atlas_width;

    std::vector<rnu::rect2f> bounds;
    std::vector<glyph_info> infos;
    auto const scale = base_size / font.units_per_em();

    for (auto p : unicode_ranges)
    {
      for (char16_t x = p.first; x <= p.second; ++x)
      {
        auto const gly = _font->glyph(x);
        if (gly == goop::glyph_id::missing)
          continue;

        auto r = _font->get_rect(gly);
        r.size *= scale;
        r.size += 2 * sdf_width;
        r.position *= scale;

        auto const defb = r;

        r.size.x = std::ceilf(r.position.x + r.size.x) - r.position.x;
        r.size.y = std::ceilf(r.position.y + r.size.y) - r.position.y;
        r.position.x = std::floorf(r.position.x);
        r.position.y = std::floorf(r.position.y);

        rnu::rect2f const err{
          .position = r.position - defb.position,
          .size = r.size - defb.size
        };

        bounds.push_back(std::move(r));
        infos.push_back(glyph_info{ x, gly, scale, defb, r, err });
      }
    }

    goop::skyline_packer skyline;
    _height = skyline.pack(bounds, _width);

    for (int i = 0; i < infos.size(); ++i)
    {
      auto info = infos[i];
      info.packed_bounds = bounds[i];
      _infos[info.id] = info;
    }
  }
  goop::texture const& sdf_font_base::atlas_texture()
  {
    if (!_texture)
    {
      std::vector<std::uint8_t> grayscale_data;
      int width = 0;
      int height = 0;

      dump(grayscale_data, width, height);

      goop::texture result;
      result->allocate(goop::texture_type::t2d, goop::data_type::r8unorm, width, height, 1);
      result->set_data(0, 0, 0, width, height, 1, grayscale_data);
      _texture = std::move(result);
    }

    return _texture.value();
  }
  void sdf_font_base::dump(std::vector<std::uint8_t>& image, int& w, int& h) const
  {
    w = _width;
    h = _height + 1;
    image.resize(w * h);

    std::for_each(std::execution::par_unseq, begin(_infos), end(_infos), [this, w, h, &image](std::pair<glyph_id, glyph_info> const& pair) {

      auto const& [ch, info] = pair;
      auto const& [character, id, scale, db, pb, err] = info;
      auto const& outline = load_glyph(pair.second.id);

      auto const at = [&](int x, int y) -> std::uint8_t& { return image[x + y * w]; };

      auto const min_x = pb.position.x;
      auto const min_y = pb.position.y;
      auto const max_x = pb.position.x + pb.size.x;
      auto const max_y = pb.position.y + pb.size.y;

      auto const voff = -pb.position + db.position + err.position - rnu::vec2(_sdf_width, _sdf_width);

      for (int i = min_x; i <= max_x; ++i)
      {
        for (int j = min_y; j <= max_y; ++j)
        {
          auto const signed_distance = goop::lines::signed_distance(outline, { (i + voff.x) / scale, (j + voff.y) / scale }) * scale;
          auto min = -_sdf_width;
          auto max = _sdf_width;

          auto const iix = 0 + i;
          auto const iiy = 0 + j;

          // Todo: letters should never overlap.
          //std::unique_lock lock(image_mutex);
          at(iix, iiy) = std::uint8_t(
            (1 - std::clamp((signed_distance - min) / (max - min), 0.0f, 1.0f)) * 255
          );
        }
      }
      });
  }
  std::vector<goop::lines::line> const& sdf_font_base::load_glyph(glyph_id glyph) const
  {
    static thread_local std::vector<goop::outline_segment> outline;
    outline.clear();
    static thread_local std::vector<goop::lines::line> letter;
    letter.clear();

    rnu::rect2f bounds;
    struct
    {
      void operator()(goop::line const& line)
      {
        goop::lines::subsample(goop::lines::line{
          .start = line.start,
          .end = line.end
          }, 3, letter);
      }
      void operator()(goop::bezier const& line)
      {
        goop::lines::subsample(goop::lines::bezier{
          .start = line.start,
          .control = line.control,
          .end = line.end
          }, 3, letter);
      }
    } visitor;

    _font->outline(glyph, bounds, [&](goop::outline_segment const& o) { std::visit(visitor, o); });

    return letter;
  }
  std::optional<glyph_id> sdf_font_base::ligature(goop::font const& font, goop::font_feature_info const& lig_feature, std::span<goop::glyph_id const> glyphs)
  {
    auto const feat_result = font.lookup_substitution(lig_feature, glyphs);
    if (!feat_result)
      return std::nullopt;

    auto const count = font.substitution_count(*feat_result);
    if (count != 1)
      return std::nullopt;

    return font.substitution_glyph(*feat_result, 0);
  }
  std::vector<sdf_font_base::set_glyph_t> sdf_font_base::text_set(std::wstring_view str, int *num_lines)
  {
    rnu::vec2 cursor{ 0, 0 };
    auto ligature_feature = _font->query_feature(goop::font_feature_type::substitution, goop::font_script::scr_latin, goop::font_language::lang_default, goop::font_feature::ft_liga);
    auto kerning_feature = _font->query_feature(goop::font_feature_type::positioning, goop::font_script::scr_latin, goop::font_language::lang_default, goop::font_feature::ft_kern);

    thread_local static std::vector<std::vector<goop::glyph_id>> glyph_lines;
    glyph_lines.clear();
    glyph_lines.emplace_back();
    for (auto& c : str)
    {
      if (c == '\n')
      {
        glyph_lines.emplace_back();
        continue;
      }
      glyph_lines.back().push_back(_font->glyph(c));
    }

    bool has_substituted = true;
    auto const font_scale = _base_size / _font->units_per_em();

    while (has_substituted)
    {
      has_substituted = false;
      for (auto& glyphs : glyph_lines)
      {
        for (int i = 0; i < glyphs.size() - 1; ++i)
        {
          auto const second = glyphs.size() - 1 - i;
          auto const first = second - 1;

          auto sub = (i < glyphs.size() - 2) && ligature_feature ? ligature(*_font, *ligature_feature, std::array{ glyphs[first - 1], glyphs[first], glyphs[second] }) : std::nullopt;
          if (!sub && ligature_feature)
            sub = ligature(*_font, *ligature_feature, std::array{ glyphs[first], glyphs[second] });
          if (sub)
          {
            has_substituted = true;
            glyphs[first] = *sub;
            glyphs.erase(glyphs.begin() + second);
          }
        }
      }
    }

    std::vector<set_glyph_t> set_glyphs(std::accumulate(begin(glyph_lines), end(glyph_lines), 0ull, [](auto const& val, auto& v) { return val + v.size(); }));

    auto const basey = 40;
    auto const rad = 0.5f;

    int base_i = 0;
    float base_x = cursor.x;
    for (auto const& glyphs : glyph_lines)
    {
      for (int i = 0; i < glyphs.size(); ++i)
      {
        auto const gly = glyphs[i];
        auto& result = set_glyphs[base_i + i];
        result.glyph = gly;
        auto const rec = _font->get_rect(gly);

        auto const next_glyph = i < glyphs.size() - 1 ? glyphs[i + 1] : goop::glyph_id::missing;

        auto const kerning = !kerning_feature ? std::nullopt : _font->lookup_positioning(*kerning_feature, std::array{ gly, next_glyph });

        auto const kerning_values = !kerning ? nullptr : std::get_if<font_accessor::pair_value_feature>(&*kerning);

        auto [ad1, be1] = _font->advance_bearing(gly);

        // Kerning somewhat broken in Roboto-Regular between K and r
        if (kerning_values)
        {
          ad1 += kerning_values->first.x_advance;
          be1 += kerning_values->first.x_placement;
        }

        auto const& info = _infos[gly];

        result.bounds.size = rec.size * font_scale;
        result.bounds.position = cursor + rnu::vec2((be1 + rec.position.x) * font_scale, rec.position.y * font_scale);
        result.bounds.position -= _sdf_width;
        result.bounds.size += 2 * _sdf_width;

        auto const scale_by = 1.0 / rnu::vec2(_width, _height);
        result.uvs = info.packed_bounds;
        result.uvs.position -= info.error.position;
        result.uvs.size -= info.error.size;
        result.uvs.position *= scale_by;
        result.uvs.size *= scale_by;

        cursor.x += ad1 * font_scale;
      }
      base_i += glyphs.size();
      cursor.x = base_x;
      cursor.y -= (_font->ascent() - _font->descent()) * font_scale;
      if (num_lines) ++*num_lines;
    }

    return set_glyphs;
  }
  float sdf_font_base::base_size() const {
    return _base_size;
  }
  float sdf_font_base::sdf_width() const {
    return _sdf_width;
  }
  float sdf_font_base::em_factor(float target_size) const {
    return target_size / _base_size;
  }
  float sdf_font_base::line_height() const
  {
    return (_font->ascent() - _font->descent()) * _base_size / _font->units_per_em();
  }
  font const& sdf_font_base::font() const
  {
    return _font.value();
  }
}