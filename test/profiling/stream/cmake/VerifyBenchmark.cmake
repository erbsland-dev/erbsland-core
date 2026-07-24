execute_process(
        COMMAND "${EXECUTABLE}" --config "${CONFIGURATION}"
        RESULT_VARIABLE benchmark_result
        OUTPUT_VARIABLE benchmark_output
)
if(NOT benchmark_result EQUAL 0)
    message(FATAL_ERROR "The focused benchmark failed.")
endif()
foreach(required_text
        "samples=5"
        "median-ns="
        "mean-ns="
        "p95-ns="
        "min-mib-per-second="
        "median-mib-per-second="
        "mean-mib-per-second="
        "p95-mib-per-second="
        "max-mib-per-second="
        "fairness-min-mib-per-second="
        "fairness-max-mib-per-second="
        "fairness-cv="
        "timeouts=")
    string(FIND "${benchmark_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Benchmark output is missing '${required_text}'.")
    endif()
endforeach()
