# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-network-interop)
erbsland_set_required_compiler_options(erbsland-core-network-interop)
target_link_libraries(erbsland-core-network-interop PRIVATE erbsland::core)
target_include_directories(erbsland-core-network-interop PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/test/unittest/src
        src
)
target_compile_definitions(erbsland-core-network-interop PRIVATE ERBSLAND_UNITTEST_BUILD=1)
target_compile_definitions(
        erbsland-core-network-interop
        PRIVATE
        ERBSLAND_CORE_INTEROP_CA_FILE="${PROJECT_SOURCE_DIR}/test/unittest/data/network/tls-interop/ca.pem")

add_subdirectory(src)

erbsland_core_configure_unittest(erbsland-core-network-interop)

find_program(ERBSLAND_CORE_CARGO_EXECUTABLE cargo REQUIRED)
set(_rust_source_directory "${CMAKE_CURRENT_SOURCE_DIR}/rust")
set(ERBSLAND_CORE_CARGO_TARGET_DIRECTORY "" CACHE PATH "Optional guest-local Cargo target directory")
if(ERBSLAND_CORE_CARGO_TARGET_DIRECTORY)
    set(_rust_target_directory "${ERBSLAND_CORE_CARGO_TARGET_DIRECTORY}")
else()
    set(_rust_target_directory "${CMAKE_CURRENT_BINARY_DIR}/cargo")
endif()
if(WIN32)
    set(_rust_executable "${_rust_target_directory}/release/erbsland-core-network-interop-counterpart.exe")
else()
    set(_rust_executable "${_rust_target_directory}/release/erbsland-core-network-interop-counterpart")
endif()
add_custom_target(erbsland-core-network-interop-counterpart
        COMMAND "${ERBSLAND_CORE_CARGO_EXECUTABLE}" build --release --locked
                --manifest-path "${_rust_source_directory}/Cargo.toml"
                --target-dir "${_rust_target_directory}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${_rust_executable}"
                "$<TARGET_FILE_DIR:erbsland-core-network-interop>"
        DEPENDS
                "${_rust_source_directory}/Cargo.toml"
                "${_rust_source_directory}/Cargo.lock"
                "${_rust_source_directory}/src/main.rs"
        BYPRODUCTS "${_rust_executable}"
        WORKING_DIRECTORY "${_rust_source_directory}"
        VERBATIM
        COMMENT "Building the rustls network interop counterpart"
)
add_dependencies(erbsland-core-network-interop erbsland-core-network-interop-counterpart)

find_program(ERBSLAND_CORE_OPENSSL_EXECUTABLE NAMES openssl)
set(_openssl_is_supported OFF)
if(ERBSLAND_CORE_OPENSSL_EXECUTABLE)
    execute_process(
            COMMAND "${ERBSLAND_CORE_OPENSSL_EXECUTABLE}" version
            OUTPUT_VARIABLE _openssl_version
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(_openssl_version MATCHES "^OpenSSL (1\\.1\\.1|[3-9])")
        set(_openssl_is_supported ON)
        target_compile_definitions(
                erbsland-core-network-interop
                PRIVATE ERBSLAND_CORE_OPENSSL_EXECUTABLE="${ERBSLAND_CORE_OPENSSL_EXECUTABLE}")
    else()
        message(STATUS "Network interop: skipping OpenSSL s_client (${_openssl_version}; TLS 1.3 OpenSSL required)")
    endif()
else()
    message(STATUS "Network interop: skipping OpenSSL s_client (executable not found)")
endif()

find_program(ERBSLAND_CORE_CURL_EXECUTABLE NAMES curl)
set(_curl_is_supported OFF)
if(ERBSLAND_CORE_CURL_EXECUTABLE)
    execute_process(
            COMMAND "${ERBSLAND_CORE_CURL_EXECUTABLE}" --version
            OUTPUT_VARIABLE _curl_version
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _curl_version MATCHES "SecureTransport")
        set(_curl_is_supported ON)
        target_compile_definitions(
                erbsland-core-network-interop
                PRIVATE ERBSLAND_CORE_CURL_EXECUTABLE="${ERBSLAND_CORE_CURL_EXECUTABLE}")
    else()
        message(STATUS "Network interop: skipping curl HTTPS (SecureTransport build has no TLS 1.3 control)")
    endif()
else()
    message(STATUS "Network interop: skipping curl HTTPS (executable not found)")
endif()

if(ERBSLAND_CORE_REQUIRE_EXTERNAL_TLS_TOOLS AND
        (NOT _openssl_is_supported OR NOT _curl_is_supported))
    message(FATAL_ERROR "Required current OpenSSL and curl TLS interoperability tools are unavailable")
endif()

add_custom_command(TARGET erbsland-core-network-interop POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "$<TARGET_FILE_DIR:erbsland-core-network-interop>/data/network/tls-interop"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${PROJECT_SOURCE_DIR}/test/unittest/data/network/tls-interop/ca.pem"
                "${PROJECT_SOURCE_DIR}/test/unittest/data/network/tls-interop/server.pem"
                "${PROJECT_SOURCE_DIR}/test/unittest/data/network/tls-interop/server-key.pem"
                "${PROJECT_SOURCE_DIR}/test/unittest/data/network/tls-interop/server-ecdsa.pem"
                "${PROJECT_SOURCE_DIR}/test/unittest/data/network/tls-interop/server-ecdsa-key.pem"
                "${PROJECT_SOURCE_DIR}/test/unittest/data/network/tls-interop/server-ed25519.pem"
                "${PROJECT_SOURCE_DIR}/test/unittest/data/network/tls-interop/server-ed25519-key.pem"
                "$<TARGET_FILE_DIR:erbsland-core-network-interop>/data/network/tls-interop"
        VERBATIM
)

if(WIN32)
    target_compile_definitions(erbsland-core-network-interop PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(erbsland-core-network-interop PRIVATE ws2_32)
endif()
