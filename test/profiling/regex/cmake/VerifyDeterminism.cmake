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
    message(FATAL_ERROR "A deterministic regex profiler dry run failed.")
endif()
if(NOT first_output STREQUAL second_output)
    message(FATAL_ERROR
            "Repeated regex profiler dry runs produced different output.\nFirst:\n${first_output}\nSecond:\n${second_output}")
endif()
string(FIND "${first_output}" "config-md5=" digest_at)
if(digest_at EQUAL -1)
    message(FATAL_ERROR "Deterministic regex profiler output has no configuration digest.")
endif()
