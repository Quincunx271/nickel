find_package(Python3 COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv --help
  RESULT_VARIABLE NO_VENV
  OUTPUT_QUIET
  ERROR_QUIET)

if(NO_VENV)
  message(FATAL_ERROR "The benchmarks require the python installation ${Python3_EXECUTABLE} to support venv.")
endif()

# Set up a venv in the binary directory.
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/.venv.done)
  file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/venv)
  execute_process(COMMAND "${Python3_EXECUTABLE}" -m venv ${CMAKE_CURRENT_BINARY_DIR}/venv)
  file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/.venv.done)
endif()

# Install chevron in the venv
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/.venv.chevron.done)
  execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/venv/bin/pip3 install chevron)
  file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/.venv.chevron.done)
endif()

# Install matplotlib in the venv
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/.venv.matplotlib.done)
execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/venv/bin/pip3 install matplotlib)
file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/.venv.matplotlib.done)
endif()

set(ARG_VALUES 0 10 20 30 40 50 60 70 80 90 100 150 200)
set(REPEAT_COUNT 200)

set(BENCHMARK_LIST ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "" FORCE)
set(BENCHMARK_PY ${CMAKE_CURRENT_BINARY_DIR}/venv/bin/python3 CACHE PATH "" FORCE)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BENCHMARK_LIST}/bench.py)
define_property(TARGET
  PROPERTY BUILDBENCH_BENCHMARKS
  BRIEF_DOCS "The list of benchmarks built by buildbench"
  FULL_DOCS "The list of benchmarks built by buildbench"
)

add_custom_target(buildbench
  COMMAND
    ${BENCHMARK_PY} ${BENCHMARK_LIST}/benchrunner.py
    "$<TARGET_PROPERTY:buildbench,BUILDBENCH_BENCHMARKS>"
    ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} -j1 --target
  COMMENT "Running benchmarks"
  VERBATIM
  USES_TERMINAL
)

function(add_build_benchmarks dir link)
  get_filename_component(dir ${dir} ABSOLUTE)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/buildbench)
  
  file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/buildbench/bench.cpp)
  string(MAKE_C_IDENTIFIER "buildbench-${dir}-lib" BenchCompileTarget)
  set(BenchCleanTarget ${BenchCompileTarget}-clean)
  set(BenchGeneratedFile ${CMAKE_CURRENT_BINARY_DIR}/buildbench/bench.cpp)
  add_library(${BenchCompileTarget} OBJECT EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/buildbench/bench.cpp)
  target_link_libraries(${BenchCompileTarget} PRIVATE ${link})
  add_custom_target(${BenchCleanTarget}
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_OBJECTS:${BenchCompileTarget}>
    COMMENT "Cleaning benchmark"
  )

  execute_process(COMMAND ${BENCHMARK_PY} ${BENCHMARK_LIST}/bench.py
    generate ${dir} -o ${CMAKE_CURRENT_BINARY_DIR}/buildbench/Benchmarks.cmake
  )
  include(${CMAKE_CURRENT_BINARY_DIR}/buildbench/Benchmarks.cmake)
endfunction()

add_build_benchmarks(buildbench nickel::nickel)
file(GLOB_RECURSE benchs CONFIGURE_DEPENDS "buildbench/*.bench")

add_custom_target(buildbench-visualize
  COMMAND ${BENCHMARK_PY} ${BENCHMARK_LIST}/visualize.py ${CMAKE_CURRENT_BINARY_DIR}/buildbench/bench.results.pickle
)
