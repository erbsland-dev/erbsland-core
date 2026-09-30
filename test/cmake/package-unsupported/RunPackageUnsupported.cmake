# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

foreach(required IN ITEMS PACKAGE_SOURCE_DIRECTORY PACKAGE_TEST_DIRECTORY PACKAGE_GENERATOR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "Missing package test argument: ${required}")
    endif()
endforeach()

set(fixture "${PACKAGE_SOURCE_DIRECTORY}/test/cmake/package-unsupported")
set(build_directory "${PACKAGE_TEST_DIRECTORY}/build")
set(install_directory "${PACKAGE_TEST_DIRECTORY}/installed")
file(REMOVE_RECURSE "${PACKAGE_TEST_DIRECTORY}")

execute_process(
        COMMAND "${CMAKE_COMMAND}" -S "${fixture}" -B "${build_directory}" -G "${PACKAGE_GENERATOR}"
                -DCMAKE_BUILD_TYPE=Release "-DPACKAGE_SOURCE_DIRECTORY=${PACKAGE_SOURCE_DIRECTORY}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Fixture configuration failed:\n${output}\n${error}")
endif()
if(NOT error MATCHES "erbsland_core_package: no release package is created")
    message(FATAL_ERROR "Expected unsupported-platform warning:\n${output}\n${error}")
endif()
execute_process(
        COMMAND "${CMAKE_COMMAND}" -S "${fixture}" -B "${PACKAGE_TEST_DIRECTORY}/invalid-name-build"
                -G "${PACKAGE_GENERATOR}" -DCMAKE_BUILD_TYPE=Release
                "-DPACKAGE_SOURCE_DIRECTORY=${PACKAGE_SOURCE_DIRECTORY}" -DPACKAGE_INVALID_NAME=ON
        RESULT_VARIABLE invalid_name_result OUTPUT_VARIABLE invalid_name_output ERROR_VARIABLE invalid_name_error)
if(invalid_name_result EQUAL 0 OR
        NOT "${invalid_name_output}${invalid_name_error}" MATCHES "Package NAME must be a regular ELCL name")
    message(FATAL_ERROR "CMake accepted an invalid package NAME:\n${invalid_name_output}\n${invalid_name_error}")
endif()
execute_process(
        COMMAND "${CMAKE_COMMAND}" -S "${fixture}" -B "${PACKAGE_TEST_DIRECTORY}/invalid-target-build"
                -G "${PACKAGE_GENERATOR}" -DCMAKE_BUILD_TYPE=Release
                "-DPACKAGE_SOURCE_DIRECTORY=${PACKAGE_SOURCE_DIRECTORY}" -DPACKAGE_INVALID_TARGET=ON
        RESULT_VARIABLE invalid_target_result OUTPUT_VARIABLE invalid_target_output ERROR_VARIABLE invalid_target_error)
if(invalid_target_result EQUAL 0 OR
        NOT "${invalid_target_output}${invalid_target_error}" MATCHES "Package target name must be a regular ELCL name")
    message(FATAL_ERROR "CMake accepted an invalid package target:\n${invalid_target_output}\n${invalid_target_error}")
endif()

execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${build_directory}" --target package_unsupported --config Release
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Fixture build failed:\n${output}\n${error}")
endif()

execute_process(
        COMMAND "${CMAKE_COMMAND}" --install "${build_directory}" --config Release --prefix "${install_directory}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Fixture install failed:\n${output}\n${error}")
endif()

if(WIN32)
    set(executable_suffix ".exe")
else()
    set(executable_suffix "")
endif()
if(NOT EXISTS "${install_directory}/bin/package_unsupported${executable_suffix}")
    message(FATAL_ERROR "The native install rule did not install the executable.")
endif()
file(GLOB archives "${PACKAGE_TEST_DIRECTORY}/*.zip" "${PACKAGE_TEST_DIRECTORY}/packages/*.zip")
if(archives)
    message(FATAL_ERROR "The unsupported platform created a ZIP: ${archives}")
endif()
