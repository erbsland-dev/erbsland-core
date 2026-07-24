execute_process(
        COMMAND "${EXECUTABLE}" --config "${CONFIGURATION}" --threads "${THREADS}"
        RESULT_VARIABLE smoke_result
        OUTPUT_VARIABLE smoke_output
)
if(NOT smoke_result EQUAL 0)
    message(FATAL_ERROR "The ${THREADS}-thread smoke workload failed.")
endif()
foreach(required_text
        "covered-scenarios=29"
        "validation=md5-ok"
        "method=read-byte"
        "method=read-span"
        "method=read-block"
        "method=read-exact"
        "method=read-all"
        "method=write-byte"
        "method=write-span"
        "method=write-block"
        "method=read-char"
        "method=read-text"
        "method=read-line"
        "method=write-char"
        "method=write-text"
        "method=write-line"
        "file-type=binary"
        "file-type=utf8"
        "file-type=utf16"
        "file-type=utf32")
    string(FIND "${smoke_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Smoke output is missing '${required_text}'.")
    endif()
endforeach()
string(REGEX MATCHALL "validation=md5-ok" validation_records "${smoke_output}")
list(LENGTH validation_records validation_count)
if(NOT validation_count EQUAL 29)
    message(FATAL_ERROR "Expected 29 MD5 validation records, received ${validation_count}.")
endif()
