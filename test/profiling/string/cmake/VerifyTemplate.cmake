execute_process(
        COMMAND "${EXECUTABLE}" --write-template "${OUTPUT}"
        RESULT_VARIABLE template_result
)
if(NOT template_result EQUAL 0 OR NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "String profiler failed to write its configuration template.")
endif()
file(READ "${OUTPUT}" actual_template)
file(READ "${EXPECTED}" expected_template)
file(REMOVE "${OUTPUT}")
if(NOT actual_template STREQUAL expected_template)
    message(FATAL_ERROR "Written string profiler template differs from the embedded default.")
endif()
