# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

# Local harness implementation shared by the application and its deterministic unit tests.
add_library(erbsland-constant-time STATIC EXCLUDE_FROM_ALL)
erbsland_core_setup_static_library(TARGET erbsland-constant-time)
erbsland_enable_debug_warnings(erbsland-constant-time)
target_include_directories(erbsland-constant-time PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${PROJECT_SOURCE_DIR}/src"
)

# Preserve the opaque checksum boundary across translation units and prevent cross-unit optimization.
set_target_properties(erbsland-constant-time PROPERTIES
        UNITY_BUILD OFF
        INTERPROCEDURAL_OPTIMIZATION OFF
)

add_subdirectory(src)

# Compile the existing pinned RSA key into the library's resources. Staging selects only this fixture;
# the resulting application has no runtime dependency on a PEM file or the source/build directory.
set(constant_time_resource_directory "${CMAKE_CURRENT_BINARY_DIR}/fixtures")
configure_file(
        "${PROJECT_SOURCE_DIR}/test/unittest/data/cryptology/signing/rsa-2048-pkcs8.pem"
        "${constant_time_resource_directory}/rsa-2048-pkcs8.pem"
        COPYONLY
)
erbsland_core_add_resources(
        TARGET erbsland-constant-time
        DIRECTORY "${constant_time_resource_directory}"
        IDENTIFIER constant-time-fixtures
        SUFFIXES .pem
)
