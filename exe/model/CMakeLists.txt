add_executable(model model.cpp "vec.cpp" "text.hpp" "text.cpp" "sdf_font.cpp")
target_link_libraries(model PRIVATE goop)