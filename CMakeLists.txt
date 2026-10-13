cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(chippie LANGUAGES C CXX)

include(FetchContent)

FetchContent_Declare(raylib
  GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
  GIT_TAG "6.0"
  GIT_SHALLOW ON
)

FetchContent_MakeAvailable(raylib)

add_executable(chippie)
target_compile_features(chippie PRIVATE cxx_std_23)
target_link_libraries(chippie PRIVATE raylib)
target_sources(chippie PRIVATE
  src/main.cpp
  src/CPU.cpp
)

add_subdirectory("chipc")
