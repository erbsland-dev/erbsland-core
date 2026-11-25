# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28...3.29)

# Each part publishes its include root and the libraries used by its public headers.
function(erbsland_core_add_tool_library part)
    set(target "erbsland-core-tool-${part}")
    add_library("${target}" STATIC)
    erbsland_core_setup_static_library(TARGET "${target}")
    target_include_directories("${target}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    foreach(dependency IN LISTS ARGN)
        target_link_libraries("${target}" PUBLIC "erbsland-core-tool-${dependency}")
    endforeach()
endfunction()

add_subdirectory(lib/frame)
add_subdirectory(lib/headers)
add_subdirectory(lib/cleanup)
add_subdirectory(lib/anti_patterns)
add_subdirectory(lib/doc)
add_subdirectory(lib/elcl_format)
add_subdirectory(lib/github_workflows)
add_subdirectory(lib/include_dependencies)
add_subdirectory(lib/pre_commit)
add_subdirectory(lib/security_hashes)

if(ERBSLAND_CORE_ENABLE_TOOLS)
    add_subdirectory(lib/core)
    set(core_tool_parts
            erbsland-core-tool-anti_patterns
            erbsland-core-tool-cleanup
            erbsland-core-tool-core
            erbsland-core-tool-doc
            erbsland-core-tool-elcl_format
            erbsland-core-tool-frame
            erbsland-core-tool-github_workflows
            erbsland-core-tool-headers
            erbsland-core-tool-include_dependencies
            erbsland-core-tool-pre_commit
            erbsland-core-tool-security_hashes
    )
    add_executable(erbsland-core-tool)
    target_link_libraries(erbsland-core-tool PRIVATE ${core_tool_parts})
    erbsland_core_setup_application(TARGET erbsland-core-tool)
    add_subdirectory(app/src)
    if(ERBSLAND_CORE_ENABLE_TESTS)
        add_subdirectory(test/clang-format-fixture)
        add_subdirectory(test/sphinx-fixture)
        add_subdirectory(test/capture-fixture)
        add_subdirectory(test/maintenance-fixture)
        add_subdirectory(test/unittest)
    endif()
endif()
