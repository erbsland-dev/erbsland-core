execute_process(
        COMMAND "${EXECUTABLE}" --list-coverage
        RESULT_VARIABLE coverage_result
        OUTPUT_VARIABLE coverage_output
)
if(NOT coverage_result EQUAL 0)
    message(FATAL_ERROR "Listing compression profiler coverage failed.")
endif()
foreach(required_text
        "functionality=compress api=compression::ByteCompressor::compress"
        "functionality=decompress api=compression::ByteDecompressor::decompress")
    string(FIND "${coverage_output}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Compression profiler coverage is missing '${required_text}'.\n${coverage_output}")
    endif()
endforeach()

execute_process(
        COMMAND "${EXECUTABLE}" --dry-run
        RESULT_VARIABLE snapshot_result
        OUTPUT_VARIABLE snapshot_output
)
execute_process(
        COMMAND "${EXECUTABLE}" --suite levels --dry-run
        RESULT_VARIABLE levels_result
        OUTPUT_VARIABLE levels_output
)
execute_process(
        COMMAND "${EXECUTABLE}" --suite formats --dry-run
        RESULT_VARIABLE formats_result
        OUTPUT_VARIABLE formats_output
)
execute_process(
        COMMAND "${EXECUTABLE}" --suite full --dry-run
        RESULT_VARIABLE full_result
        OUTPUT_VARIABLE full_output
)
if(NOT snapshot_result EQUAL 0 OR NOT levels_result EQUAL 0 OR NOT formats_result EQUAL 0 OR NOT full_result EQUAL 0)
    message(FATAL_ERROR "Expanding compression profiler suites failed.")
endif()

foreach(algorithm "lz4-block" "deflate" "bzip2" "lzma" "zstandard")
    foreach(functionality "compress" "decompress")
        string(FIND "${snapshot_output}" "snapshot:${functionality}:${algorithm}:raw:default:mixed" found_at)
        if(found_at EQUAL -1)
            message(FATAL_ERROR "Snapshot suite is missing ${functionality}/${algorithm}.\n${snapshot_output}")
        endif()
        foreach(level "fastest" "fast" "default" "high" "highest")
            string(FIND "${levels_output}" "levels:${functionality}:${algorithm}:raw:${level}:mixed" found_at)
            if(found_at EQUAL -1)
                message(FATAL_ERROR "Levels suite is missing ${functionality}/${algorithm}/${level}.\n${levels_output}")
            endif()
        endforeach()
        foreach(format "raw" "core")
            string(FIND "${formats_output}" "formats:${functionality}:${algorithm}:${format}:default:structured" found_at)
            if(found_at EQUAL -1)
                message(FATAL_ERROR "Formats suite is missing ${functionality}/${algorithm}/${format}.\n${formats_output}")
            endif()
        endforeach()
    endforeach()
endforeach()

foreach(corpus "prose" "structured" "source" "runs" "matches" "sparse-binary" "entropy" "mixed")
    string(FIND "${snapshot_output}" "snapshot:compress:deflate:raw:default:${corpus}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR "Snapshot suite is missing corpus '${corpus}'.\n${snapshot_output}")
    endif()
endforeach()

foreach(algorithm "deflate" "bzip2" "lzma" "zstandard")
    foreach(functionality "compress" "decompress")
        string(FIND "${formats_output}" "formats:${functionality}:${algorithm}:zip:default:structured" found_at)
        if(found_at EQUAL -1)
            message(FATAL_ERROR "Formats suite is missing ZIP ${functionality}/${algorithm}.\n${formats_output}")
        endif()
    endforeach()
endforeach()
string(FIND "${formats_output}" ":lz4-block:zip:" invalid_lz4_zip_at)
if(NOT invalid_lz4_zip_at EQUAL -1)
    message(FATAL_ERROR "Formats suite contains the unsupported LZ4/ZIP combination.\n${formats_output}")
endif()
string(FIND "${full_output}" "record=summary action=dry-run scenarios=1120" full_count_at)
if(full_count_at EQUAL -1)
    message(FATAL_ERROR "Full compression suite does not contain the complete compatible matrix.\n${full_output}")
endif()

execute_process(
        COMMAND "${EXECUTABLE}" --suite windows --dry-run
        RESULT_VARIABLE windows_result
        OUTPUT_VARIABLE windows_output
)
if(NOT windows_result EQUAL 0 OR NOT windows_output MATCHES "record=summary action=dry-run scenarios=120 ")
    message(FATAL_ERROR "Window suite does not contain the native codec matrix.\n${windows_output}")
endif()
foreach(algorithm "deflate" "bzip2" "lzma" "zstandard")
    foreach(functionality "compress" "decompress")
        foreach(level "fastest" "fast" "default" "high" "highest")
            foreach(corpus "matches" "entropy" "mixed")
                string(FIND "${windows_output}" "windows:${functionality}:${algorithm}:raw:${level}:${corpus}" found_at)
                if(found_at EQUAL -1)
                    message(FATAL_ERROR "Window suite is missing ${functionality}/${algorithm}/${level}/${corpus}.")
                endif()
            endforeach()
        endforeach()
    endforeach()
endforeach()
