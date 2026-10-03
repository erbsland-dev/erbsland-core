# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
set(test_directory "${BUILD_DIRECTORY}/test/cmake/display-text-core-only")
file(REMOVE_RECURSE "${test_directory}")

function(run)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Command failed (${result}): ${ARGV}\n${output}\n${error}")
    endif()
endfunction()

foreach(extension IN ITEMS hpp cpp)
    set(catalog "${SOURCE_DIRECTORY}/src/erbsland/i18n/impl/DisplayTextCore.${extension}")
    file(SHA256 "${catalog}" before_${extension})
    file(TIMESTAMP "${catalog}" time_${extension} "%s.%f")
endforeach()
run("${CMAKE_COMMAND}" -S "${SOURCE_DIRECTORY}" -B "${test_directory}" -G "${GENERATOR}"
        -DCMAKE_BUILD_TYPE=Debug -DCMAKE_UNITY_BUILD=ON
        -DERBSLAND_CORE_ENABLE_TESTS=OFF -DERBSLAND_CORE_ENABLE_DEMOS=OFF
        -DERBSLAND_CORE_ENABLE_TOOLS=OFF -DERBSLAND_CORE_DEVELOPER_BUILD=OFF
        -DERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS=OFF)
run("${CMAKE_COMMAND}" --build "${test_directory}" --config Debug --target erbsland-core --parallel 8)
foreach(extension IN ITEMS hpp cpp)
    set(catalog "${SOURCE_DIRECTORY}/src/erbsland/i18n/impl/DisplayTextCore.${extension}")
    file(SHA256 "${catalog}" after)
    file(TIMESTAMP "${catalog}" after_time "%s.%f")
    if(NOT after STREQUAL before_${extension} OR NOT after_time STREQUAL time_${extension})
        message(FATAL_ERROR "Core-only build modified the checked-in catalog.")
    endif()
endforeach()
foreach(tool IN ITEMS display-text-compiler resource-compiler)
    file(GLOB_RECURSE executable "${test_directory}/cmake/tools/${tool}/erbsland-core-${tool}"
            "${test_directory}/cmake/tools/${tool}/erbsland-core-${tool}.exe")
    if(executable)
        message(FATAL_ERROR "Core-only build unexpectedly built ${tool}.")
    endif()
endforeach()
if(EXISTS "${test_directory}/cmake-tools")
    message(FATAL_ERROR "Core-only build created a bootstrap build.")
endif()

# Merely configuring Core for a cross build must not require either native compiler.
run("${CMAKE_COMMAND}" -S "${SOURCE_DIRECTORY}" -B "${test_directory}/cross" -G "${GENERATOR}"
        "-DCMAKE_SYSTEM_NAME=${CMAKE_HOST_SYSTEM_NAME}"
        -DERBSLAND_CORE_ENABLE_TESTS=OFF -DERBSLAND_CORE_ENABLE_DEMOS=OFF
        -DERBSLAND_CORE_ENABLE_TOOLS=OFF)
