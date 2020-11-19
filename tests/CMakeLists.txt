find_package(Catch2 2.5.0 REQUIRED)

# Set up warnings / similar flags
set(MSVC_flags /permissive-)
set(Clang_flags -Wall -Wextra -Wpedantic)
set(GNU_flags -Wall -Wextra -Wpedantic)

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "*.test.cpp")

add_executable(test.nickel ${test_sources})
target_link_libraries(test.nickel
  PRIVATE
    nickel::nickel
    Catch2::Catch2
)
target_compile_options(test.nickel
  PRIVATE
    ${${CMAKE_CXX_COMPILER_ID}_flags}
)
target_compile_features(test.nickel
  PRIVATE
    cxx_std_17
)

include(Catch)

catch_discover_tests(test.nickel
  EXTRA_ARGS $<$<BOOL:${CHEF_TEST_COLOR}>:--use-colour=yes>
)
