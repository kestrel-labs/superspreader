cmake_minimum_required(VERSION 3.22.1)

project(
  "superspreader"
  VERSION 1.0.0
  DESCRIPTION "Code for the game: \"Super Spreader\""
  LANGUAGES CXX
)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    add_compile_options(-Werror -Wall -Wextra -Wconversion -Wold-style-cast)
endif()

# Default to Release build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Use O3 on Release mode
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#############################################################################
# Super Spreader Libraries
#############################################################################

add_library(superspreader_health_monitor
  health_monitor/health_monitor_core.cpp
)
target_compile_features(superspreader_health_monitor PUBLIC cxx_std_14)
target_include_directories(superspreader_health_monitor PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/health_monitor>
)

#############################################################################
# Testing
#############################################################################

include(CTest)
if(BUILD_TESTING)
    enable_testing()

    # Download and unpack googletest at configure time
    configure_file(cmake/third_party/googletest.cmake googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
      RESULT_VARIABLE result
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
    if(result)
      message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
      RESULT_VARIABLE result
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
    if(result)
      message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif()

    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This defines
    # the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                     ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                     EXCLUDE_FROM_ALL)

    # Add tests
    add_subdirectory(test)
endif()
