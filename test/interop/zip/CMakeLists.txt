# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-zip-interop)
erbsland_set_required_compiler_options(erbsland-core-zip-interop)
target_link_libraries(erbsland-core-zip-interop PRIVATE erbsland::core)
target_compile_definitions(erbsland-core-zip-interop PRIVATE ERBSLAND_UNITTEST_BUILD=1)

add_subdirectory(src)

erbsland_core_configure_unittest(erbsland-core-zip-interop
        COPY_TEST_DATA vectors
        ENABLE_DATA_DEPS
)

find_program(ERBSLAND_CORE_CARGO_EXECUTABLE cargo REQUIRED)
set(_rust_source_directory "${CMAKE_CURRENT_SOURCE_DIR}/rust")
set(ERBSLAND_CORE_CARGO_TARGET_DIRECTORY "" CACHE PATH "Optional guest-local Cargo target directory")
if(ERBSLAND_CORE_CARGO_TARGET_DIRECTORY)
    set(_rust_target_directory "${ERBSLAND_CORE_CARGO_TARGET_DIRECTORY}")
else()
    set(_rust_target_directory "${CMAKE_CURRENT_BINARY_DIR}/cargo")
endif()
if(WIN32)
    set(_rust_executable "${_rust_target_directory}/release/erbsland-core-zip-interop-counterpart.exe")
else()
    set(_rust_executable "${_rust_target_directory}/release/erbsland-core-zip-interop-counterpart")
endif()
add_custom_target(erbsland-core-zip-interop-counterpart
        COMMAND "${ERBSLAND_CORE_CARGO_EXECUTABLE}" build --release --locked
                --manifest-path "${_rust_source_directory}/Cargo.toml"
                --target-dir "${_rust_target_directory}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${_rust_executable}"
                "$<TARGET_FILE_DIR:erbsland-core-zip-interop>"
        DEPENDS
                "${_rust_source_directory}/Cargo.toml"
                "${_rust_source_directory}/Cargo.lock"
                "${_rust_source_directory}/src/main.rs"
        BYPRODUCTS "${_rust_executable}"
        WORKING_DIRECTORY "${_rust_source_directory}"
        VERBATIM
        COMMENT "Building the ZIP interop counterpart"
)
add_dependencies(erbsland-core-zip-interop erbsland-core-zip-interop-counterpart)
