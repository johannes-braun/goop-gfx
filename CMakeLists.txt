# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.20)

project ("goop")

find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(Stb REQUIRED)
find_package(Stb REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
add_subdirectory(rnu)

find_path(STB_INCLUDE_DIR stb_image.h)

# Include sub-projects.
add_subdirectory ("goop")
add_subdirectory ("exe")
