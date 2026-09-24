# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-compression-interop)
erbsland_set_required_compiler_options(erbsland-core-compression-interop)
target_link_libraries(erbsland-core-compression-interop PRIVATE erbsland::core)
target_compile_definitions(erbsland-core-compression-interop PRIVATE ERBSLAND_UNITTEST_BUILD=1)

add_subdirectory(src)

configure_file(
        vectors/manifest.tsv
        compression-conformance-vectors.tsv
        COPYONLY
)

erbsland_core_configure_unittest(erbsland-core-compression-interop)

find_program(ERBSLAND_CORE_CARGO_EXECUTABLE cargo REQUIRED)
set(_rust_source_directory "${CMAKE_CURRENT_SOURCE_DIR}/rust")
set(ERBSLAND_CORE_CARGO_TARGET_DIRECTORY "" CACHE PATH "Optional guest-local Cargo target directory")
if(ERBSLAND_CORE_CARGO_TARGET_DIRECTORY)
    set(_rust_target_directory "${ERBSLAND_CORE_CARGO_TARGET_DIRECTORY}")
else()
    set(_rust_target_directory "${CMAKE_CURRENT_BINARY_DIR}/cargo")
endif()
if(WIN32)
    set(_rust_executable
            "${_rust_target_directory}/release/erbsland-core-compression-interop-counterpart.exe")
else()
    set(_rust_executable
            "${_rust_target_directory}/release/erbsland-core-compression-interop-counterpart")
endif()
add_custom_target(erbsland-core-compression-interop-counterpart
        COMMAND "${ERBSLAND_CORE_CARGO_EXECUTABLE}" build --release --locked
                --manifest-path "${_rust_source_directory}/Cargo.toml"
                --target-dir "${_rust_target_directory}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${_rust_executable}"
                "$<TARGET_FILE_DIR:erbsland-core-compression-interop>"
        DEPENDS
                "${_rust_source_directory}/Cargo.toml"
                "${_rust_source_directory}/Cargo.lock"
                "${_rust_source_directory}/src/main.rs"
        BYPRODUCTS "${_rust_executable}"
        WORKING_DIRECTORY "${_rust_source_directory}"
        VERBATIM
        COMMENT "Building the compression interop counterpart"
)
add_dependencies(erbsland-core-compression-interop erbsland-core-compression-interop-counterpart)
