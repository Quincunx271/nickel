find_package(Catch2 2.5.0 REQUIRED)

# Set up warnings / similar flags
set(MSVC_flags /permissive-)
set(Clang_flags -Wall -Wextra -Wpedantic)
set(GNU_flags -Wall -Wextra -Wpedantic)

add_executable(test.nickel
  catch_main.test.cpp
  # sources
  nickel/nickel.test.cpp
)
target_link_libraries(test.nickel
  PRIVATE
    nickel::nickel
    Catch2::Catch2
)
target_compile_options(test.nickel
  PRIVATE
    ${${CMAKE_CXX_COMPILER_ID}_flags}
)

include(Catch)

catch_discover_tests(test.nickel
  EXTRA_ARGS $<$<BOOL:${CHEF_TEST_COLOR}>:--use-colour=yes>
)
