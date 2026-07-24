execute_process(
        COMMAND "${EXECUTABLE}" --config "${CONFIGURATION}" --threads "${THREADS}"
        RESULT_VARIABLE smoke_result
        OUTPUT_VARIABLE smoke_output
)
if(NOT smoke_result EQUAL 0)
    message(FATAL_ERROR "The ${THREADS}-thread string profiler smoke workload failed.\n${smoke_output}")
endif()
foreach(required_text
        "covered-scenarios=6"
        "width=u8"
        "width=u16"
        "width=u32"
        "type=string"
        "type=string-editor"
        "sensitive=normal"
        "sensitive=sensitive"
        "median-ns-per-operation="
        "median-code-points-per-second="
        "fairness-cv=")
    string(FIND "${smoke_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "String profiler smoke output is missing '${required_text}'.\n${smoke_output}")
    endif()
endforeach()
