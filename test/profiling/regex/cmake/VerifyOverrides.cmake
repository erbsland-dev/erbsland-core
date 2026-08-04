# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

execute_process(
        COMMAND "${EXECUTABLE}"
                --suite comparison
                --mode benchmark
                --threads 2
                --duration 2s
                --seed 17
                --warmup-samples 0
                --samples 2
                --minimum-sample-time 1ms
                --memory-limit "32 MiB"
                --progress-interval 1s
                --dry-run
        RESULT_VARIABLE override_result
        OUTPUT_VARIABLE override_output
)
if(NOT override_result EQUAL 0)
    message(FATAL_ERROR "Regex profiler command-line override validation failed.\n${override_output}")
endif()

foreach(required_text
        "action=dry-run scenarios=10"
        "threads=2"
        "suite=comparison"
        "duration-ns=2000000000"
        "seed=17"
        "warmup-samples=0"
        "samples=2"
        "minimum-sample-time-ns=1000000"
        "memory-limit=33554432"
        "progress-interval-ns=1000000000")
    string(FIND "${override_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Regex profiler override output is missing '${required_text}'.\n${override_output}")
    endif()
endforeach()
