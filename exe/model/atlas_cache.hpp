#pragma once
#include <filesystem>
#include <vector>
#include <span>

namespace goop::gui
{
  class atlas_cache
  {
  public:
    struct data_type
    {
      int width;
      int height;
      int components;
      std::vector<std::uint8_t> data;
    };

    atlas_cache(std::filesystem::path file_base);

    std::optional<data_type> load_data(std::size_t current_hash);
    void save_data(std::size_t hash, std::span<std::uint8_t const> data, int w, int h, int c);

  private:
    std::filesystem::path _file_base;
    std::filesystem::path _image_file;
    std::filesystem::path _info_file;
  };
}