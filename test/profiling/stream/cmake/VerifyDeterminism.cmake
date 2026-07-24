execute_process(
        COMMAND "${EXECUTABLE}" --config "${CONFIGURATION}" --dry-run
        RESULT_VARIABLE first_result
        OUTPUT_VARIABLE first_output
)
execute_process(
        COMMAND "${EXECUTABLE}" --config "${CONFIGURATION}" --dry-run
        RESULT_VARIABLE second_result
        OUTPUT_VARIABLE second_output
)
if(NOT first_result EQUAL 0 OR NOT second_result EQUAL 0)
    message(FATAL_ERROR "The deterministic dry-run command failed.")
endif()
if(NOT first_output STREQUAL second_output)
    message(FATAL_ERROR "Two dry runs produced different file assignments or seeds.")
endif()
foreach(required_text
        "assignment-kind=shared"
        "assignment-kind=unique"
        "chunk-mode=fixed"
        "chunk-mode=incrementing"
        "chunk-mode=random"
        "dry-run scenarios=70"
        "files-per-worker="
        "maximum-active-corpus-bytes=")
    string(FIND "${first_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Dry-run output is missing '${required_text}'.")
    endif()
endforeach()
