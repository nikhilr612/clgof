cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    clgof
    VERSION 0.1.0
    DESCRIPTION "Conway's game of life implemented using OpenCL"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Deps ----
find_package(OpenCL REQUIRED)
find_package(SFML 2.5 REQUIRED graphics window system)
# ---- Declare library ----

add_library(
    clgof_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    clgof_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(clgof_lib PUBLIC cxx_std_17)

# ---- Declare executable ----

add_executable(clgof_exe source/main.cpp)
add_executable(clgof::exe ALIAS clgof_exe)

set_property(TARGET clgof_exe PROPERTY OUTPUT_NAME clgof)

target_compile_features(clgof_exe PRIVATE cxx_std_17)

target_link_libraries(clgof_exe PRIVATE clgof_lib OpenCL::OpenCL sfml-graphics sfml-window sfml-system)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT clgof_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of clgof"
  )
endif()

include(cmake/dev-mode.cmake)
