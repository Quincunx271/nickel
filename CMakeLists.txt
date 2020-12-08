cmake_minimum_required(VERSION 3.14)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(nickel
  VERSION 0.0.1
  LANGUAGES CXX
  DESCRIPTION "C++ Named Parameters"
)

include(is_root)
is_root(is_root_project)

set(NICKEL_DEVELOPER_DEFAULTS "${is_root_project}" CACHE BOOL "Default all options to developer-friendly values")

option(BUILD_TESTING "Enable testing" ${NICKEL_DEVELOPER_DEFAULTS})
option(BUILD_BENCHMARKS "Builds the benchmarks" OFF)
option(NICKEL_TEST_COLOR "Force test color" OFF)

if(BUILD_TESTING)
  enable_testing()

  # For Catch2:
  include(pmm.cmake)
  pmm(CONAN)
endif()

add_library(nickel INTERFACE)
add_library(nickel::nickel ALIAS nickel)
target_include_directories(nickel
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(nickel
  INTERFACE
    cxx_std_14
)

# Tests
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

# Install
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/nickel")

install(TARGETS nickel
  EXPORT nickel-Targets
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(EXPORT nickel-Targets
  FILE nickelTargets.cmake
  NAMESPACE nickel::
  DESTINATION "${INSTALL_CONFIGDIR}"
)

install(DIRECTORY include/nickel/
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/nickel"
  FILES_MATCHING PATTERN "*.hpp"
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/nickelConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMinorVersion
  ARCH_INDEPENDENT
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/nickelConfigVersion.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nickelConfig.cmake"
  DESTINATION "${INSTALL_CONFIGDIR}"
)
