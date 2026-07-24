execute_process(
        COMMAND "${EXECUTABLE}" --config "${CONFIGURATION}" --threads "${THREADS}"
        RESULT_VARIABLE smoke_result
        OUTPUT_VARIABLE smoke_output
)
if(NOT smoke_result EQUAL 0)
    message(FATAL_ERROR "The ${THREADS}-thread byte profiler smoke workload failed.\n${smoke_output}")
endif()
foreach(required_text
        "covered-scenarios=9"
        "type=byte-array"
        "type=byte-block"
        "type=byte-block-editor"
        "type=byte-buffer"
        "type=byte-ring-buffer"
        "sensitive=normal"
        "sensitive=sensitive"
        "median-ns-per-operation="
        "fairness-cv=")
    string(FIND "${smoke_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Byte profiler smoke output is missing '${required_text}'.\n${smoke_output}")
    endif()
endforeach()
