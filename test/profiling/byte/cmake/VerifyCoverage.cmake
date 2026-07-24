execute_process(
        COMMAND "${EXECUTABLE}" --list-coverage
        RESULT_VARIABLE coverage_result
        OUTPUT_VARIABLE coverage_output
)
if(NOT coverage_result EQUAL 0)
    message(FATAL_ERROR "Listing byte profiler coverage failed.")
endif()
foreach(required_text
        "record=summary action=coverage paths="
        "type=byte-array"
        "type=byte-block"
        "type=byte-block-editor"
        "type=byte-buffer"
        "type=byte-ring-buffer"
        "use-case=secure-erase"
        "use-case=cow-stress"
        "use-case=edit-stress"
        "use-case=ring-wrapped-cycle"
        "apis=")
    string(FIND "${coverage_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Coverage output is missing '${required_text}'.")
    endif()
endforeach()
