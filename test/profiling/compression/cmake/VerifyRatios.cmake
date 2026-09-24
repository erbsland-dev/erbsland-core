# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

execute_process(
        COMMAND "${EXECUTABLE}" --suite snapshot --algorithm zstandard --corpus mixed --corpus entropy
                --samples 1 --warmup-samples 0 --minimum-sample-time 100us
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Compression ratio profiling failed.\n${output}")
endif()
string(REGEX MATCHALL "record=compression-size [^\n]+" records "${output}")
list(LENGTH records record_count)
if(NOT record_count EQUAL 4)
    message(FATAL_ERROR "Expected one ratio record per compression/decompression fixture.\n${output}")
endif()
foreach(record IN LISTS records)
    if(NOT record MATCHES "input-bytes=([0-9]+) compressed-bytes=([0-9]+) size-ratio=([0-9]+)\\.([0-9]+) savings-percent=(-?[0-9.]+) reduced=(true|false)")
        message(FATAL_ERROR "Incomplete compression ratio record: ${record}")
    endif()
    set(input "${CMAKE_MATCH_1}")
    set(compressed "${CMAKE_MATCH_2}")
    set(whole "${CMAKE_MATCH_3}")
    set(fraction "${CMAKE_MATCH_4}000000")
    set(reduced "${CMAKE_MATCH_6}")
    string(SUBSTRING "${fraction}" 0 6 fraction)
    # Prefixing the fraction avoids interpreting leading zeroes as octal.
    math(EXPR actual "${whole} * 1000000 + 1${fraction} - 1000000")
    math(EXPR expected "${compressed} * 1000000 / ${input}")
    math(EXPR difference "${actual} - ${expected}")
    if(difference LESS -1 OR difference GREATER 1)
        message(FATAL_ERROR "Reported size ratio disagrees with byte counts: ${record}")
    endif()
    if(record MATCHES ":mixed ")
        math(EXPR maximum "${input} / 2")
        if(NOT compressed LESS maximum OR NOT reduced STREQUAL "true")
            message(FATAL_ERROR "Zstandard stopped reducing the compressible fixture: ${record}")
        endif()
    elseif(compressed LESS input OR NOT reduced STREQUAL "false")
        message(FATAL_ERROR "Entropy fixture should report expansion: ${record}")
    endif()
endforeach()
