# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

# Compile only: linker failures must never count as rejected construction.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
function(erbsland_core_check_unit_construction name should_compile body)
    try_compile(compiled
            SOURCE_FROM_CONTENT "${name}.cpp"
            "#include <erbsland/unit/ByteIndex.hpp>\n#include <erbsland/unit/ByteCount.hpp>\n#include <erbsland/unit/ByteOffset.hpp>\n#include <erbsland/unit/Version.hpp>\n#include <limits>\nusing namespace erbsland::unit;\n${body}\n"
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${PROJECT_SOURCE_DIR}/include;${PROJECT_SOURCE_DIR}/src"
            COMPILE_DEFINITIONS -DERBSLAND_CORE_BUILDING_LIBRARY=1
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED TRUE
            OUTPUT_VARIABLE output)
    if(should_compile AND NOT compiled)
        message(FATAL_ERROR "Unit construction positive control ${name} failed:\n${output}")
    elseif(NOT should_compile AND compiled)
        message(FATAL_ERROR "Unit construction ${name} unexpectedly compiled")
    endif()
endfunction()

erbsland_core_check_unit_construction(valid TRUE
        "static_assert(ByteIndex(42) == ByteIndex{42}); static_assert(ByteCount{UINT64_MAX-1}.isMaximum()); static_assert(ByteOffset{INT64_MIN}.isMinimum()); static_assert(Major(65535).isMaximum()); constexpr auto v = Version{1,2,3,4}; auto runtime(int n) { return ByteCount::fromInteger(n); }")
foreach(expression IN ITEMS
        "ByteIndex{-1}" "ByteIndex(-1)" "ByteCount{-1}" "ByteCount(-1)"
        "ByteIndex{UINT64_MAX}" "ByteIndex(UINT64_MAX)" "ByteCount{UINT64_MAX}" "ByteCount(UINT64_MAX)"
        "ByteOffset{UINT64_MAX}" "ByteOffset(UINT64_MAX)"
        "Major{-1}" "Major(-1)" "Major{65536}" "Major(65536)" "Version{65536}" "Version(-1)"
        "ByteCount{1} * 1" "ByteOffset{1} * 1U" "Major{1} + 1"
        "ByteIndex::fromInteger(1.5)" "ByteCount::fromInteger(true)")
    string(MAKE_C_IDENTIFIER "${expression}" name)
    erbsland_core_check_unit_construction("invalid_${name}" FALSE "auto value = ${expression};")
endforeach()
foreach(type IN ITEMS ByteIndex ByteCount ByteOffset Major Version)
    erbsland_core_check_unit_construction("float_${type}" FALSE "auto value = ${type}(1.5);")
    erbsland_core_check_unit_construction("float_braces_${type}" FALSE "auto value = ${type}{1.5};")
    erbsland_core_check_unit_construction("bool_${type}" FALSE "auto value = ${type}{true};")
    erbsland_core_check_unit_construction("runtime_${type}" FALSE "auto make(int value) { return ${type}(value); }")
    erbsland_core_check_unit_construction("runtime_braces_${type}" FALSE "auto make(int value) { return ${type}{value}; }")
endforeach()
