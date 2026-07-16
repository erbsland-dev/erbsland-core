# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.23)
include_guard()

function(erbsland_set_required_compiler_options target)
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
    target_compile_features(${target} PUBLIC cxx_std_20)
    # The project does not use C++ modules. Disable dependency scanning explicitly so CMake can group sources into
    # unity batches on compilers where module scanning is enabled by policy.
    set_target_properties(${target} PROPERTIES CXX_SCAN_FOR_MODULES OFF)

    if(MSVC)
        target_compile_options(${target} PUBLIC /utf-8)
        target_compile_options(${target} PRIVATE /MP /bigobj)
    else()
        target_compile_options(${target} PRIVATE
                $<$<CONFIG:Debug>:-Wno-trigraphs>
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Unity sources expose compiler-generated coroutine frame types across merged translation units. GCC's
        # subobject-linkage diagnostic is not actionable for these implementation-generated types.
        target_compile_options(${target} PRIVATE
                $<$<BOOL:$<TARGET_PROPERTY:UNITY_BUILD>>:-Wno-subobject-linkage>
        )
    endif()

    if(ERBSLAND_CORE_DO_NOT_FLATTEN_NS)
        target_compile_definitions(${target} PUBLIC -DERBSLAND_CORE_DO_NOT_FLATTEN_NS=1)
    endif()
    if(ERBSLAND_CORE_DEVELOPER_BUILD)
        target_compile_definitions(${target} PUBLIC $<$<CONFIG:Debug>:-DERBSLAND_CORE_DEVELOPER_BUILD=1>)
    endif()
endfunction()

function(erbsland_enable_precompiled_headers target)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs GNU_EXCLUDED_SOURCES)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS)
        return()
    endif()

    target_precompile_headers(${target} PRIVATE
            <algorithm>
            <array>
            <atomic>
            <chrono>
            <compare>
            <concepts>
            <cstddef>
            <cstdint>
            <exception>
            <format>
            <functional>
            <limits>
            <memory>
            <optional>
            <span>
            <string>
            <string_view>
            <tuple>
            <type_traits>
            <utility>
            <variant>
            <vector>
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND ARGS_GNU_EXCLUDED_SOURCES)
        # GCC can become pathologically slow when template-heavy sources use the standard-library PCH. Keep these
        # exceptions out of unity batches so regular and unity builds remain practical.
        set_source_files_properties(${ARGS_GNU_EXCLUDED_SOURCES} PROPERTIES
                SKIP_PRECOMPILE_HEADERS TRUE
                SKIP_UNITY_BUILD_INCLUSION TRUE
        )
    endif()
endfunction()
