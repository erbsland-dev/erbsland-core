# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

file(MAKE_DIRECTORY "${WORK_DIRECTORY}")

function(run_profile)
    execute_process(
            COMMAND "${EXECUTABLE}" ${ARGN}
            WORKING_DIRECTORY "${WORK_DIRECTORY}"
            RESULT_VARIABLE result
            OUTPUT_VARIABLE output
            ERROR_VARIABLE errors
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Profiling failed (${result}).\n${output}\n${errors}")
    endif()
    set(profile_output "${output}" PARENT_SCOPE)
endfunction()

function(require_output expression)
    if(NOT profile_output MATCHES "${expression}")
        message(FATAL_ERROR "Missing '${expression}' in:\n${profile_output}")
    endif()
endfunction()

function(write_scenario path root method)
    file(WRITE "${path}"
            "@version: \"1.0\"\n"
            "---[ Run ]---\nMode: \"benchmark\"\nDuration: 60s\nThreads: 1\n"
            "Samples: 1\nWarmup Samples: 0\nMinimum Sample Time: 1us\n"
            "---*[ Scenario ]*---\nName: \"source-smoke\"\n"
            "Functionality: \"${method}\"\nRoot: \"${root}\"\n")
endfunction()

if(CHECK STREQUAL "registration")
    run_profile(--suite source-tree --dry-run)
    require_output("scenarios=6")
    foreach(directory demos src test)
        foreach(method text data)
            require_output("id=source-tree:${directory}:${method}")
        endforeach()
    endforeach()
    run_profile(--suite source-tree --scenario source-tree:src:text --list-scenarios)
    require_output("id=source-tree:src:text")
    require_output("scenarios=1")
    run_profile(--list-coverage)
    require_output("functionality=walk-read-text")
    require_output("functionality=walk-read-data")
    return()
endif()

if(CHECK STREQUAL "errors")
    foreach(kind missing empty)
        if(kind STREQUAL "missing")
            set(root "${WORK_DIRECTORY}/does-not-exist")
            set(expected "is not a directory")
        else()
            # This existing directory contains configuration files, but no C++ sources.
            set(root "${SOURCE_DIRECTORY}/config")
            set(expected "contains no selected C\\+\\+ files")
        endif()
        set(configuration "${WORK_DIRECTORY}/${kind}.elcl")
        write_scenario("${configuration}" "${root}" "walk-read-data")
        execute_process(
                COMMAND "${EXECUTABLE}" --config "${configuration}"
                RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors
        )
        string(REGEX REPLACE "[ \t\r\n]+" " " diagnostic "${errors}")
        if(result EQUAL 0 OR NOT diagnostic MATCHES "${expected}")
            message(FATAL_ERROR "Expected ${kind}-root diagnostic.\n${output}\n${errors}")
        endif()
    endforeach()
    return()
endif()

if(CHECK STREQUAL "traversal")
    foreach(method path-callback path-info-callback std-recursive)
        set(configuration "${WORK_DIRECTORY}/${method}.elcl")
        write_scenario("${configuration}" "${SOURCE_DIRECTORY}/src" "${method}")
        run_profile(--config "${configuration}")
        require_output("record=benchmark")
        require_output(" files=0 ")
        require_output(" returned-bytes=0 ")
    endforeach()
    return()
endif()

set(threads 1)
if(CHECK STREQUAL "read-two")
    set(threads 2)
endif()

# Independent counts from actual profiler sources; no generated file corpus.
file(GLOB_RECURSE source_files LIST_DIRECTORIES FALSE
        "${SOURCE_DIRECTORY}/src/*.hpp" "${SOURCE_DIRECTORY}/src/*.cpp" "${SOURCE_DIRECTORY}/src/*.tpp")
set(expected_files 0)
set(expected_bytes 0)
foreach(path IN LISTS source_files)
    if(NOT IS_SYMLINK "${path}")
        math(EXPR expected_files "${expected_files} + 1")
        file(SIZE "${path}" bytes)
        math(EXPR expected_bytes "${expected_bytes} + ${bytes}")
    endif()
endforeach()
math(EXPR measured_files "${expected_files} * ${threads}")
math(EXPR measured_bytes "${expected_bytes} * ${threads}")

foreach(method text data)
    set(configuration "${WORK_DIRECTORY}/${method}.elcl")
    write_scenario("${configuration}" "${SOURCE_DIRECTORY}/src" "walk-read-${method}")
    run_profile(--config "${configuration}" --dry-run)
    require_output("scenarios=1")
    run_profile(--config "${configuration}" --threads "${threads}")
    require_output("record=corpus[^\n]* files=${expected_files} input-bytes=${expected_bytes} ")
    require_output("record=benchmark[^\n]* files=${measured_files} ")
    require_output("files-per-second=[1-9][0-9]*")
    require_output("record=summary")
    if(method STREQUAL "data")
        require_output("returned-byte-kind=file-data")
        require_output("record=benchmark[^\n]* returned-bytes=${measured_bytes} ")
    else()
        require_output("returned-byte-kind=decoded-utf8")
        require_output("record=benchmark[^\n]* returned-bytes=[1-9][0-9]* ")
    endif()
endforeach()
