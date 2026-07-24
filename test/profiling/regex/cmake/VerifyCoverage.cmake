execute_process(
        COMMAND "${EXECUTABLE}" --list-coverage
        RESULT_VARIABLE coverage_result
        OUTPUT_VARIABLE coverage_output
)
if(NOT coverage_result EQUAL 0)
    message(FATAL_ERROR "Listing regex profiler coverage failed.")
endif()
foreach(required_text
        "record=summary action=coverage paths=44"
        "use-case=compile"
        "use-case=lazy-compile"
        "use-case=lazy-first-use"
        "use-case=lazy-contended-first-use"
        "input=string-utf16"
        "input=string-utf32"
        "input=file-utf8"
        "input=file-utf16"
        "input=file-utf32"
        "api=RegEx::replaceAll(expression)"
        "api=RegEx::replaceAll(callback)")
    string(FIND "${coverage_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Regex profiler coverage output is missing '${required_text}'.\n${coverage_output}")
    endif()
endforeach()
