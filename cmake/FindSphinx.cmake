find_program(Sphinx_EXECUTABLE
  NAMES sphinx-build
  REQUIRED
)

if(NOT Sphinx_FIND_QUIET)
  message(STATUS "Found sphinx-build -- ${Sphinx_EXECUTABLE}")
endif()

execute_process(COMMAND ${Sphinx_EXECUTABLE} --version
  OUTPUT_VARIABLE Sphinx_VERSION_MESSAGE
)

if(NOT Sphinx_VERSION_MESSAGE MATCHES "sphinx-build ([0-9]+\\.[0-9]+\\.[0-9]+)")
  message(FATAL_ERROR "Unable to parse sphinx version \"${Sphinx_VERSION_MESSAGE}\"")
endif()
set(Sphinx_VERSION ${CMAKE_MATCH_1})

function(_find_sphinx_report)
  if(Sphinx_FIND_REQUIRED)
    message(SEND_ERROR ${ARGN})
  elseif(NOT Sphinx_FIND_QUIET)
    message(STATUS ${ARGN})
  endif()
endfunction()

if(Sphinx_VERSION VERSION_LESS Sphinx_FIND_VERSION)
  _find_sphinx_report("Found sphinx of version ${Sphinx_VERSION}, but required ${Sphinx_FIND_VERSION}")
  set(Sphinx_FOUND FALSE)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sphinx
  FOUND_VAR Sphinx_FOUND
  VERSION_VAR ${Sphinx_VERSION}
  REQUIRED_VARS Sphinx_EXECUTABLE Sphinx_VERSION
)

if(_find_sphinx_executed_before)
  return()
endif()
set(_find_sphinx_executed_before TRUE)

function(add_sphinx_target NAME)
  cmake_parse_arguments(PARSE_ARGV 1 ARG "ALL" "CONF;BUILDER" "SETTINGS")

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown arguments ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  set(target_args)
  set(cmd_args)
  if(ARG_ALL)
    list(APPEND target_args ALL)
  endif()

  if(ARG_CONF)
    list(APPEND cmd_args -c ${ARG_CONF})
  endif()

  if(NOT ARG_BUILDER)
    set(ARG_BUILDER html)
  endif()
  list(APPEND cmd_args -b ${ARG_BUILDER})

  foreach(setting IN LISTS ARG_SETTINGS)
    list(APPEND cmd_args -D ${setting})
  endforeach()

  list(APPEND cmd_args
    -d ${CMAKE_CURRENT_BINARY_DIR}/${ARG_BUILDER}.doctrees
    -W
    -n
    -j auto
    -a
    -q
  )

  add_custom_target(${NAME} ${target_args}
    COMMAND ${Sphinx_EXECUTABLE} ${cmd_args} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${ARG_BUILDER}
    COMMENT "Generating documentation using Sphinx"
  )
  add_custom_target(${NAME}-clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/${ARG_BUILDER}
  )
  set_property(TARGET ${NAME}
    PROPERTY
      ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${ARG_BUILDER}
  )
  set_property(TARGET ${NAME}
    PROPERTY
      SPHINX_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${ARG_BUILDER}
  )
endfunction()
