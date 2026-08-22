# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(http1-request-fuzz Http1RequestFuzz.cpp)
target_link_libraries(http1-request-fuzz PRIVATE erbsland::core)
target_include_directories(http1-request-fuzz PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(http1-request-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(http1-request-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
erbsland_set_required_compiler_options(http1-request-fuzz)
