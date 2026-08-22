# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(http1-response-fuzz Http1ResponseFuzz.cpp)
target_link_libraries(http1-response-fuzz PRIVATE erbsland::core)
target_include_directories(http1-response-fuzz PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(http1-response-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(http1-response-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
erbsland_set_required_compiler_options(http1-response-fuzz)
