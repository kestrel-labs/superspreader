cmake_minimum_required(VERSION 3.5)

project(
  "superspreader"
  VERSION 1.0.0
  DESCRIPTION "Code for the game: \"Super Spreader\""
)


#############################################################################
# Super Spreader Libraries
#############################################################################

include(GNUInstallDirs)

# Default to Release build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Use O3 on Release mode
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

add_library(
  "${PROJECT_NAME}_health_monitor"
  "${PROJECT_SOURCE_DIR}/health_monitor/health_monitor_core.cpp")

target_compile_features("${PROJECT_NAME}_health_monitor" PUBLIC cxx_std_14)

target_include_directories(
  "${PROJECT_NAME}_health_monitor"
  PUBLIC
  $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/health_monitor>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS "${PROJECT_NAME}_health_monitor"
        EXPORT "${PROJECT_NAME}_health_monitor_Targets"
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION
  ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(EXPORT "${PROJECT_NAME}_health_monitor_Targets"
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/health_monitor DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#############################################################################
# Testing
#############################################################################

if(BUILD_TESTS)
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
