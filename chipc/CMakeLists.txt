
include(FetchContent)

FetchContent_Declare(spdlog
  GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
  GIT_TAG "v1.17.0"
  GIT_SHALLOW ON
)

FetchContent_Declare(argparse
  GIT_REPOSITORY "https://github.com/p-ranav/argparse.git"
  GIT_TAG "v3.2"
  GIT_SHALLOW ON
)

FetchContent_Declare(ctre
  GIT_REPOSITORY
    "https://github.com/hanickadot/compile-time-regular-expressions"
  GIT_TAG "v3.11.0"
  GIT_SHALLOW ON
)

FetchContent_MakeAvailable(spdlog argparse ctre)


set(PROJ_SRC
  "main.cpp"
  "lexer.cpp"
)

list(TRANSFORM PROJ_SRC PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/src/")

add_executable(chipc)

target_sources(chipc PRIVATE ${PROJ_SRC})

set_target_properties(chipc PROPERTIES
  CMAKE_CXX_STANDARD 23
  CMAKE_CXX_STANDARD_REQUIRED ON
)

target_link_libraries(chipc PRIVATE
  argparse::argparse
  spdlog::spdlog
  ctre::ctre
)
