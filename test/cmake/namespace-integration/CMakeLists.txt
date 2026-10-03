# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

# These targets compile through the real Core usage requirements, including in top-level development builds.
add_library(erbsland-core-namespace-consumer OBJECT)
set_target_properties(erbsland-core-namespace-consumer PROPERTIES UNITY_BUILD OFF)
target_link_libraries(erbsland-core-namespace-consumer PRIVATE erbsland::core)

add_library(erbsland-core-namespace-explicit-consumer OBJECT)
target_link_libraries(erbsland-core-namespace-explicit-consumer PRIVATE erbsland::core)
target_compile_definitions(erbsland-core-namespace-explicit-consumer PRIVATE ERBSLAND_CORE_DO_NOT_FLATTEN_NS=1)

add_library(erbsland-core-namespace-library OBJECT)
target_link_libraries(erbsland-core-namespace-library PRIVATE erbsland::core)
target_compile_definitions(erbsland-core-namespace-library PRIVATE ERBSLAND_CORE_BUILDING_LIBRARY=1)

# The implementation must always be strict, even when embedded or configured for release.
get_target_property(core_definitions erbsland-core COMPILE_DEFINITIONS)
if(NOT "ERBSLAND_CORE_BUILDING_LIBRARY=1" IN_LIST core_definitions)
    message(FATAL_ERROR "Core must privately enable strict library namespaces.")
endif()

# Compile failures verify the boundary itself, rather than just the presence of a build definition.
function(erbsland_core_check_namespace_failure name source definition diagnostic)
    try_compile(compiled
            SOURCE_FROM_CONTENT "${name}.cpp" "${source}"
            CMAKE_FLAGS
                "-DINCLUDE_DIRECTORIES=${PROJECT_SOURCE_DIR}/include;${PROJECT_SOURCE_DIR}/src"
            COMPILE_DEFINITIONS "-D${definition}=1"
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED TRUE
            OUTPUT_VARIABLE output
    )
    if(compiled OR NOT output MATCHES "${diagnostic}")
        message(FATAL_ERROR "Namespace check ${name} did not fail as expected:\n${output}")
    endif()
endfunction()

# Avoid link failures obscuring the expected compiler diagnostics.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
foreach(definition IN ITEMS ERBSLAND_CORE_BUILDING_LIBRARY ERBSLAND_CORE_DO_NOT_FLATTEN_NS)
    erbsland_core_check_namespace_failure("${definition}-flattened"
            "#include <erbsland/String.hpp>\nerbsland::String *value;\n"
            "${definition}" "String")
    erbsland_core_check_namespace_failure("${definition}-leaf"
            "#include <erbsland/CompressionFormat.hpp>\nerbsland::CompressionFormat *value;\n"
            "${definition}" "CompressionFormat")
    erbsland_core_check_namespace_failure("${definition}-aggregate"
            "#include <erbsland/core/all.hpp>\nerbsland::Application *value;\n"
            "${definition}" "Application")
endforeach()
erbsland_core_check_namespace_failure("core-direct"
        "#include <erbsland/core/impl/MakeOneNamespace.hpp>\n"
        ERBSLAND_CORE_BUILDING_LIBRARY
        "Namespace flattening is forbidden")
foreach(name IN ITEMS RegExPtr ConstRegExPtr Flags Settings Match)
    erbsland_core_check_namespace_failure("regex-${name}"
            "#include <erbsland/RegEx.hpp>\nerbsland::${name} *value;\n"
            ERBSLAND_PUBLIC_HEADER_TEST
            "${name}")
endforeach()

add_subdirectory(src)
