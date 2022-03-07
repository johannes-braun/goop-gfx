#include "shader.hpp"

namespace goop
{
  bool shader_base::load_source(shader_type type, std::string_view source, std::string* info_log)
  {
    auto const next_hash = std::hash<std::string_view>{}(source);

  if (next_hash != _hash)
  {
    return compile_glsl(type, source, info_log);
  }

  if (info_log)
    *info_log = "Shader source did not change, reusing last compilation product.";

  return true;
}

std::size_t shader_base::hash() const
{
  return _hash;
}
}