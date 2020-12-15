find_package(Catch2 2.5.0 REQUIRED)

# Set up warnings / similar flags
set(werr ${NICKEL_WARNINGS_AS_ERRORS})

if(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
  add_compile_options(
    /permissive- /W4
    $<$<BOOL:${werr}>:/WX>
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  add_compile_options(
    -Wall -Wextra -Wpedantic
    -fsanitize=address
    -fsanitize=undefined
    $<$<BOOL:${werr}>:-Werror>
  )
  add_link_options(
    -fsanitize=address
    -fsanitize=undefined
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  add_compile_options(
    -Wall -Wextra -Wpedantic
    -fsanitize=address
    -fsanitize=undefined
    $<$<BOOL:${werr}>:-Werror>
  )
  add_link_options(
    -fsanitize=address
    -fsanitize=undefined
  )
endif()

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "*.test.cpp")

add_executable(test.nickel ${test_sources})
target_link_libraries(test.nickel
  PRIVATE
    nickel::nickel
    Catch2::Catch2
)
target_compile_features(test.nickel
  PRIVATE
    cxx_std_14
)

include(Catch)

catch_discover_tests(test.nickel
  EXTRA_ARGS $<$<BOOL:${CHEF_TEST_COLOR}>:--use-colour=yes>
)
