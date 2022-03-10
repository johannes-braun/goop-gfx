# CMakeList.txt : CMake project for goop, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_library (goop STATIC
  "generic/geometry.hpp"
  "generic/geometry.cpp"  
  "generic/mapped_buffer.hpp" 
  "generic/mapped_buffer.inl.hpp" 
  "opengl/mapped_buffer.hpp" 
  "dynamic_octree.hpp"
  "dynamic_octree.inl.hpp" 
  "generic/sampler.hpp"  
  "opengl/sampler.cpp"  
  "generic/shader.cpp"
  "generic/draw_state.hpp"
  "opengl/texture.cpp"
  "generic/texture.hpp" 
  "opengl/texture.hpp"
  "graphics.hpp"
  "generic/shader.hpp"
  "opengl/shader.cpp" 
  "opengl/shader.hpp"
  "file/obj.cpp" 
  "file/obj.hpp"
  "opengl/handle.hpp"
  "opengl/handle.cpp"
  "opengl/geometry.cpp"
  "opengl/geometry.hpp"
  "opengl/draw_state.hpp"
  "opengl/draw_state.cpp"
  "generic/render_target.hpp"
  "hash.hpp"
  "generic/texture_provider.hpp"
  "opengl/render_target.hpp"
  "opengl/render_target.cpp"
  "algorithm/perlin.hpp" 
  "algorithm/looper.hpp" 
  "algorithm/looper.cpp" 
  "shadow.hpp" 
  "algorithm/cull.hpp"   
  "default_app.hpp"
  "default_app.cpp" 
  "animation/smooth.hpp" "animation/animation.hpp" "file/gltf__WIP.hpp" "file/gltf__WIP.cpp" "animation/animation.cpp")
target_compile_features(goop PUBLIC cxx_std_20)
target_link_libraries(goop PUBLIC glfw glad::glad rnu::rnu)
target_include_directories(goop PUBLIC ${STB_INCLUDE_DIR})
target_include_directories(goop PUBLIC ${TINYGLTF_INCLUDE_DIRS})
target_include_directories(goop PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(goop PUBLIC -DGLFW_INCLUDE_NONE)
