# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

function(erbsland_add_profiling_executable)
    set(_one_value TARGET DEFAULT_CONFIGURATION DEFAULT_HEADER_TEMPLATE DEFAULT_VARIABLE)
    set(_multi_value SOURCES INCLUDE_DIRECTORIES LINK_LIBRARIES)
    cmake_parse_arguments(PROFILE "" "${_one_value}" "${_multi_value}" ${ARGN})
    if(NOT PROFILE_TARGET)
        message(FATAL_ERROR "erbsland_add_profiling_executable requires TARGET")
    endif()
    if(PROFILE_DEFAULT_CONFIGURATION)
        if(NOT PROFILE_DEFAULT_HEADER_TEMPLATE OR NOT PROFILE_DEFAULT_VARIABLE)
            message(FATAL_ERROR "A default configuration requires DEFAULT_HEADER_TEMPLATE and DEFAULT_VARIABLE")
        endif()
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${PROFILE_DEFAULT_CONFIGURATION}" _configuration_text)
        set(${PROFILE_DEFAULT_VARIABLE} "${_configuration_text}")
        configure_file(
                "${PROFILE_DEFAULT_HEADER_TEMPLATE}"
                "${CMAKE_CURRENT_BINARY_DIR}/generated/DefaultConfiguration.hpp"
                @ONLY
        )
    endif()
    add_executable(${PROFILE_TARGET} ${PROFILE_SOURCES})
    target_compile_features(${PROFILE_TARGET} PRIVATE cxx_std_20)
    target_include_directories(${PROFILE_TARGET} PRIVATE
            "${CMAKE_CURRENT_BINARY_DIR}/generated"
            ${PROFILE_INCLUDE_DIRECTORIES}
    )
    target_link_libraries(${PROFILE_TARGET} PRIVATE erbsland::profiling ${PROFILE_LINK_LIBRARIES})
    erbsland_set_required_compiler_options(${PROFILE_TARGET})
    erbsland_enable_debug_warnings(${PROFILE_TARGET})
    set_target_properties(${PROFILE_TARGET} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/profiling-apps"
            FOLDER "Tests/Profiling"
    )
endfunction()
