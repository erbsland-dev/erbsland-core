# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(punycode-fuzz)
target_link_libraries(punycode-fuzz PRIVATE erbsland::core)
target_compile_options(punycode-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(punycode-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
erbsland_set_required_compiler_options(punycode-fuzz)

add_subdirectory(src)
