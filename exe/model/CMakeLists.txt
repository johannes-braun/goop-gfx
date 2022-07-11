add_executable(model model.cpp "vec.cpp" "text.hpp" "text.cpp" "sdf_font.cpp" "vector_graphics.cpp" "sdf_2d.cpp" "symbol.cpp")
target_link_libraries(model PRIVATE goop)