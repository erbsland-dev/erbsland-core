execute_process(
        COMMAND "${EXECUTABLE}" --list-coverage
        RESULT_VARIABLE coverage_result
        OUTPUT_VARIABLE coverage_output
)
if(NOT coverage_result EQUAL 0)
    message(FATAL_ERROR "Listing character-set profiler coverage failed.")
endif()
foreach(required_text
        "functionality=construct-char"
        "api=CharSet::CharSet(Char)"
        "functionality=contains"
        "functionality=united-with"
        "functionality=add-range"
        "functionality=for-each-char"
        "functionality=to-u32-string"
        "functionality=from-ascii-category"
        "functionality=from-unicode-category"
        "functionality=from-unicode-group"
        "functionality=from-pattern-u8"
        "functionality=from-pattern-u16"
        "functionality=from-pattern-u32"
        "record=summary action=coverage functionalities=60")
    string(FIND "${coverage_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Character-set coverage output is missing '${required_text}'.")
    endif()
endforeach()
