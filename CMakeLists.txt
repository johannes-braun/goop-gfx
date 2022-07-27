# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE          ${CMAKE_SOURCE_DIR}/out/bin/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL       ${CMAKE_SOURCE_DIR}/out/bin/minsizerel)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO   ${CMAKE_SOURCE_DIR}/out/bin/relwithdebinfo)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG            ${CMAKE_SOURCE_DIR}/out/bin/debug)
project ("goop")
include(cmake/AddShaderPackage.cmake)

find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(Stb REQUIRED)
find_package(nlohmann_json  REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_path(STB_INCLUDE_DIR stb_image.h)
add_subdirectory(rnu)

# Include sub-projects.
add_subdirectory ("goop")
add_subdirectory ("exe")
