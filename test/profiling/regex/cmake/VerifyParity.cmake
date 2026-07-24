execute_process(
        COMMAND "${EXECUTABLE}" --config "${CONFIGURATION}" --threads 1
        RESULT_VARIABLE parity_result
        OUTPUT_VARIABLE parity_output
)
if(NOT parity_result EQUAL 0)
    message(FATAL_ERROR "Regex profiler parity workload failed.\n${parity_output}")
endif()

foreach(use_case compile lazy-compile lazy-first-use lazy-contended-first-use)
    set(reference_signature "")
    foreach(input string-utf8 string-utf16 string-utf32)
        string(REGEX MATCH
                "benchmark scenario=coverage:${use_case}:${input}[^\n]* signature=([0-9]+)"
                signature_match "${parity_output}")
        if(signature_match STREQUAL "")
            message(FATAL_ERROR "Missing ${use_case}/${input} parity signature.\n${parity_output}")
        endif()
        if(reference_signature STREQUAL "")
            set(reference_signature "${CMAKE_MATCH_1}")
        elseif(NOT CMAKE_MATCH_1 STREQUAL reference_signature)
            message(FATAL_ERROR "Logical signature mismatch for ${use_case}/${input}.")
        endif()
    endforeach()
endforeach()

foreach(use_case match full-match find-first find-all collect-all)
    set(reference_signature "")
    foreach(input string-utf8 string-utf16 string-utf32 file-utf8 file-utf16 file-utf32)
        string(REGEX MATCH
                "benchmark scenario=coverage:${use_case}:${input}[^\n]* signature=([0-9]+)"
                signature_match "${parity_output}")
        if(signature_match STREQUAL "")
            message(FATAL_ERROR "Missing ${use_case}/${input} parity signature.\n${parity_output}")
        endif()
        if(reference_signature STREQUAL "")
            set(reference_signature "${CMAKE_MATCH_1}")
        elseif(NOT CMAKE_MATCH_1 STREQUAL reference_signature)
            message(FATAL_ERROR "Logical signature mismatch for ${use_case}/${input}.")
        endif()
    endforeach()
endforeach()
