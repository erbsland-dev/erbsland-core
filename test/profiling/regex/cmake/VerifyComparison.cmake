# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

execute_process(
        COMMAND "${EXECUTABLE}"
                --suite comparison
                --threads 1
                --warmup-samples 0
                --samples 2
                --minimum-sample-time 1ms
        RESULT_VARIABLE comparison_result
        OUTPUT_VARIABLE comparison_output
        ERROR_VARIABLE comparison_error
)
if(NOT comparison_result EQUAL 0)
    message(FATAL_ERROR
            "Regex std::regex comparison workload failed with '${comparison_result}'.\n"
            "Standard output:\n${comparison_output}\n"
            "Standard error:\n${comparison_error}")
endif()

string(REGEX MATCHALL "record=comparison[^\n]*" comparison_records "${comparison_output}")
list(LENGTH comparison_records comparison_count)
if(NOT comparison_count EQUAL 5)
    message(FATAL_ERROR "Expected five regex comparison records, got ${comparison_count}.\n${comparison_output}")
endif()

foreach(required_name
        compile-alternatives-8
        compile-http-request
        full-match-4k
        find-first-miss-64k
        find-all-words)
    string(REGEX MATCH
            "record=comparison name=${required_name}[^\n]* parity=yes"
            comparison_match "${comparison_output}")
    if(comparison_match STREQUAL "")
        message(FATAL_ERROR "Missing parity result for '${required_name}'.\n${comparison_output}")
    endif()
endforeach()

string(FIND "${comparison_output}" "compile-literal-1 " tiny_literal_at)
if(NOT tiny_literal_at EQUAL -1)
    message(FATAL_ERROR "The std::regex comparison must not use the unrepresentative one-character literal case.")
endif()
