find_package(Catch2 2.5.0 REQUIRED)

# Set up warnings / similar flags
set(werr ${NICKEL_WARNINGS_AS_ERRORS})
set(compile_options)
set(link_options)

if(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
  list(APPEND compile_options
    /permissive- /W4
    $<$<BOOL:${werr}>:/WX>
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  list(APPEND compile_options
    -Wall -Wextra -Wpedantic
    -fsanitize=address
    -fsanitize=undefined
    $<$<BOOL:${werr}>:-Werror>
  )
  list(APPEND link_options
    -fsanitize=address
    -fsanitize=undefined
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  list(APPEND compile_options
    -Wall -Wextra -Wpedantic
    -fsanitize=address
    -fsanitize=undefined
    $<$<BOOL:${werr}>:-Werror>
  )
  list(APPEND link_options
    -fsanitize=address
    -fsanitize=undefined
  )
endif()

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "nickel/*.test.cpp")

add_executable(test.nickel catch_main.test.cpp ${test_sources})
target_link_libraries(test.nickel
  PRIVATE
    nickel::nickel
    Catch2::Catch2
)
target_compile_options(test.nickel PRIVATE ${compile_options})
target_link_options(test.nickel PRIVATE ${link_options})

include(Catch)

catch_discover_tests(test.nickel
  EXTRA_ARGS $<$<BOOL:${CHEF_TEST_COLOR}>:--use-colour=yes>
)

# Test that compilation fails
include(TestCompileError)
file(GLOB_RECURSE compile_failure_test_sources CONFIGURE_DEPENDS "compile_error/*.test.cpp")

add_compile_failure_tests(
  ${compile_failure_test_sources}
  LINK_LIBRARIES nickel::nickel
)
