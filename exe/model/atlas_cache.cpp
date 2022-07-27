#include "atlas_cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stb_image.h>
#include <stb_image_write.h>

namespace goop::gui
{
  atlas_cache::atlas_cache(std::filesystem::path file_base)
    : _file_base(std::move(file_base)), _info_file(_file_base), _image_file(_file_base)
  {
    _info_file.replace_extension(".info.json");
    _image_file.replace_extension(".png");
  }

  std::optional<atlas_cache::data_type> atlas_cache::load_data(std::size_t current_hash)
  {
    if (!exists(_info_file) || !exists(_image_file))
      return std::nullopt;

    std::ifstream info_file_stream(_info_file);
    nlohmann::json info;
    info_file_stream >> info;

    auto const hash = info["hash"].get<std::size_t>();
    // cache invalid.
    if (current_hash != hash)
      return std::nullopt;

    int w, h, c;
    auto const str = _image_file.string();
    stbi_info(str.c_str(), &w, &h, &c);
    std::vector<std::uint8_t> data(w * h * c);
    std::unique_ptr<stbi_uc[], decltype(&stbi_image_free)> const loaded(
      stbi_load(str.c_str(), &w, &h, &c, 0), &stbi_image_free);
    std::copy(loaded.get(), loaded.get() + w * h * c, data.begin());
    return data_type{ w, h, c, data };
  }

  void atlas_cache::save_data(std::size_t hash, std::span<std::uint8_t const> data, int w, int h, int c)
  {
    stbi_write_png(_image_file.string().c_str(), w, h, c, data.data(), 0);
    auto const str = _image_file.string();
    std::ofstream info_file_stream(_info_file);
    info_file_stream << nlohmann::json{
      {"hash", hash}
    };
  }
}