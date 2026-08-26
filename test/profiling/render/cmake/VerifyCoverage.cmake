execute_process(
        COMMAND "${EXECUTABLE}" --list-coverage
        RESULT_VARIABLE coverage_result
        OUTPUT_VARIABLE coverage_output
)
if(NOT coverage_result EQUAL 0)
    message(FATAL_ERROR "Listing render profiler coverage failed.")
endif()
foreach(required_text
        "functionality=tokenize"
        "functionality=compile"
        "functionality=render")
    string(FIND "${coverage_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Render profiler coverage is missing '${required_text}'.\n${coverage_output}")
    endif()
endforeach()

execute_process(
        COMMAND "${EXECUTABLE}" --dry-run
        RESULT_VARIABLE dry_run_result
        OUTPUT_VARIABLE dry_run_output
)
if(NOT dry_run_result EQUAL 0)
    message(FATAL_ERROR "Expanding render profiler scenarios failed.")
endif()
execute_process(
        COMMAND "${EXECUTABLE}" --suite allocations --dry-run
        RESULT_VARIABLE allocation_dry_run_result
        OUTPUT_VARIABLE allocation_dry_run_output
)
if(NOT allocation_dry_run_result EQUAL 0)
    message(FATAL_ERROR "Expanding render allocation scenarios failed.")
endif()
string(APPEND dry_run_output "${allocation_dry_run_output}")
foreach(required_id
        "snapshot:tokenize:dashboard"
        "snapshot:tokenize:email"
        "snapshot:tokenize:report"
        "snapshot:tokenize:expression-stress"
        "snapshot:tokenize:control-stress"
        "snapshot:tokenize:text-heavy"
        "snapshot:tokenize:custom-delimiters"
        "snapshot:tokenize:include-site"
        "snapshot:tokenize:include-stress"
        "snapshot:tokenize:inheritance-site"
        "snapshot:tokenize:inheritance-stress"
        "snapshot:tokenize:language-completion.html"
        "snapshot:tokenize:legacy"
        "snapshot:tokenize:all"
        "snapshot:compile:dashboard"
        "snapshot:compile:include-site"
        "snapshot:compile:inheritance-site"
        "snapshot:compile:inheritance-stress"
        "snapshot:compile:language-completion.html"
        "snapshot:compile:legacy"
        "snapshot:compile:all"
        "snapshot:render:include-site"
        "snapshot:render:include-stress"
        "snapshot:render:inheritance-site"
        "snapshot:render:inheritance-stress"
        "snapshot:render:language-completion.html"
        "snapshot:render:legacy"
        "snapshot:render:all"
        "allocations:tokenize:language-completion.html"
        "allocations:compile:language-completion.html"
        "allocations:render:language-completion.html")
    string(FIND "${dry_run_output}" "id=${required_id}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Render profiler scenarios are missing '${required_id}'.\n${dry_run_output}")
    endif()
endforeach()
