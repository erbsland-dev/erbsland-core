# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

set(PATH_PROFILE_SOURCE_DIRECTORY "${PROJECT_SOURCE_DIR}")
configure_file(src/SourceDirectory.hpp.in generated/SourceDirectory.hpp @ONLY)

add_library(path-profile-workloads STATIC)
target_link_libraries(path-profile-workloads PUBLIC erbsland::profiling)
target_compile_features(path-profile-workloads PRIVATE cxx_std_20)
erbsland_set_required_compiler_options(path-profile-workloads)
erbsland_enable_debug_warnings(path-profile-workloads)

erbsland_add_profiling_executable(
    TARGET path-walker-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE PATH_WALKER_PROFILE_DEFAULT_CONFIGURATION
    LINK_LIBRARIES path-profile-workloads
)

add_subdirectory(src)
add_subdirectory(test)
