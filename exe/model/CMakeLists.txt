add_executable(model model.cpp "vec.cpp" "text.hpp" "text.cpp" "sdf_font.cpp" "vector_graphics.cpp" "sdf_2d.cpp" "symbol.cpp" "atlas_cache.cpp" "panel_2d.cpp" "ui/element.cpp" "ui/layout_params.cpp" "ui/frame.cpp" "ui/label.cpp")
add_subdirectory(shaders)
target_compile_definitions(model PRIVATE RESOURCE_DIRECTORY="${CMAKE_SOURCE_DIR}/res/")
target_link_libraries(model PRIVATE goop model_shaders::model_shaders)