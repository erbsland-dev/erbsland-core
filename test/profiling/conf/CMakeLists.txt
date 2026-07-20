# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(conf-parser-profile
        src/EmbeddedDocuments.cpp
        src/main.cpp
)
target_compile_features(conf-parser-profile PRIVATE cxx_std_20)
target_link_libraries(conf-parser-profile PRIVATE erbsland::core)
erbsland_set_required_compiler_options(conf-parser-profile)
erbsland_enable_debug_warnings(conf-parser-profile)
set_target_properties(conf-parser-profile PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/profiling-apps"
        FOLDER "Tests/Profiling"
)
