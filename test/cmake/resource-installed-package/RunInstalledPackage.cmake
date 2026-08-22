# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

foreach(required_variable IN ITEMS
        ERBSLAND_CORE_BUILD_DIRECTORY
        ERBSLAND_CORE_FIXTURE_DIRECTORY
        ERBSLAND_CORE_GENERATOR
        ERBSLAND_CORE_HOST_COMPILER_NAME)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing required variable: ${required_variable}")
    endif()
endforeach()

set(test_directory "${ERBSLAND_CORE_BUILD_DIRECTORY}/test/cmake/resource-installed-package")
set(install_directory "${test_directory}/install")
set(consumer_directory "${test_directory}/consumer")

file(REMOVE_RECURSE "${test_directory}")
execute_process(
        COMMAND "${CMAKE_COMMAND}" --install "${ERBSLAND_CORE_BUILD_DIRECTORY}"
                --prefix "${install_directory}" --config "${ERBSLAND_CORE_CONFIGURATION}"
        COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
        COMMAND "${CMAKE_COMMAND}"
                -S "${ERBSLAND_CORE_FIXTURE_DIRECTORY}"
                -B "${consumer_directory}"
                -G "${ERBSLAND_CORE_GENERATOR}"
                -DCMAKE_PREFIX_PATH=${install_directory}
                -DERBSLAND_CORE_RESOURCE_COMPILER_EXECUTABLE=${install_directory}/bin/${ERBSLAND_CORE_HOST_COMPILER_NAME}
        COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${consumer_directory}"
                --config "${ERBSLAND_CORE_CONFIGURATION}"
        COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
        COMMAND "${consumer_directory}/installed-resource-consumer"
        COMMAND_ERROR_IS_FATAL ANY
)
