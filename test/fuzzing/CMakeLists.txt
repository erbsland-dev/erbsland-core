# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "ERBSLAND_CORE_ENABLE_FUZZING requires upstream Clang with the libFuzzer runtime")
endif()

# Link the libFuzzer runtime only into executables. The core library must use the
# no-link variant so libFuzzer can observe coverage inside the code under test.
target_compile_options(erbsland-core PRIVATE -fsanitize=fuzzer-no-link,address,undefined)

add_subdirectory(ecdsa-signature)
add_subdirectory(http1-request)
add_subdirectory(http1-response)
add_subdirectory(punycode)
add_subdirectory(pkcs8-key)
add_subdirectory(tls-client-protocol)
add_subdirectory(tls-record)
add_subdirectory(tls-server-protocol)
add_subdirectory(x509-der)
