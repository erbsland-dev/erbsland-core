execute_process(
        COMMAND "${EXECUTABLE}" --config "${CONFIGURATION}" --threads "${THREADS}"
        RESULT_VARIABLE smoke_result
        OUTPUT_VARIABLE smoke_output
)
if(NOT smoke_result EQUAL 0)
    message(FATAL_ERROR "The ${THREADS}-thread regex profiler smoke workload failed.\n${smoke_output}")
endif()
foreach(required_text
        "covered-scenarios=44"
        "use-case=compile"
        "use-case=lazy-contended-first-use"
        "initialization-wall-median-ns="
        "use-case=match"
        "use-case=full-match"
        "use-case=find-first"
        "use-case=find-all"
        "use-case=collect-all"
        "use-case=replace-all"
        "input=string-utf8"
        "input=string-utf16"
        "input=string-utf32"
        "input=file-utf8"
        "input=file-utf16"
        "input=file-utf32"
        "replacement=expression"
        "replacement=callback"
        "median-ns-per-operation="
        "median-matches-per-second="
        "logical-code-points="
        "pattern-bytes="
        "fairness-cv=")
    string(FIND "${smoke_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Regex profiler smoke output is missing '${required_text}'.\n${smoke_output}")
    endif()
endforeach()
string(REGEX MATCH "summary[^\n]*workspace=([^ \n]+) files-kept=no" cleanup_match "${smoke_output}")
if(cleanup_match STREQUAL "")
    message(FATAL_ERROR "Regex profiler smoke output has no cleanup summary.\n${smoke_output}")
endif()
if(EXISTS "${CMAKE_MATCH_1}")
    message(FATAL_ERROR "Regex profiler left its temporary workspace at '${CMAKE_MATCH_1}'.")
endif()
