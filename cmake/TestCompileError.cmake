function(add_compile_failure_test file)
  get_filename_component(abspath ${file} ABSOLUTE)
  file(RELATIVE_PATH target_name ${CMAKE_SOURCE_DIR} ${abspath})
  string(MAKE_C_IDENTIFIER ${target_name} target_name)
  set(target_name compilefailure.test.${target_name})

  cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "LINK_LIBRARIES")

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown arguments ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  add_library(${target_name} OBJECT ${file})
  target_link_libraries(${target_name} PRIVATE ${ARG_LINK_LIBRARIES})
  set_property(TARGET ${target_name} PROPERTY EXCLUDE_FROM_ALL TRUE)
  set_property(TARGET ${target_name} PROPERTY EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  set_property(SOURCE ${file} PROPERTY COMPILE_FAILURE_TEST_TARGET ${target_name})

  add_custom_target(testrunner.${target_name}
    DEPENDS ${file}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIG> --target ${target_name}
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_OBJECTS:${target_name}>
  )
  add_test(
    NAME test.${target_name}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIG> --target testrunner.${target_name}
  )
  set_tests_properties(test.${target_name}
    PROPERTIES
      WILL_FAIL TRUE
  )
endfunction()

function(add_compile_failure_tests)
  list(FIND ARGV LINK_LIBRARIES link_libraries_index)
  list(SUBLIST ARGV 0 ${link_libraries_index} files)
  list(SUBLIST ARGV ${link_libraries_index} -1 link_libraries)

  foreach(file IN LISTS files)
    add_compile_failure_test(${file} ${link_libraries})
  endforeach()
endfunction()
