execute_process(
        COMMAND "${EXECUTABLE}" --config "${CONFIGURATION}"
        RESULT_VARIABLE smoke_result
        OUTPUT_VARIABLE smoke_output
)
if(NOT smoke_result EQUAL 0)
    message(FATAL_ERROR "The profiling smoke workload failed.\n${smoke_output}")
endif()
foreach(required_text "record=run" "record=benchmark" "worker-fairness=" "record=summary")
    string(FIND "${smoke_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Profiling smoke output is missing '${required_text}'.\n${smoke_output}")
    endif()
endforeach()
