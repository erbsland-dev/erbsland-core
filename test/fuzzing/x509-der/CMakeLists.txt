# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(x509-der-fuzz)
target_link_libraries(x509-der-fuzz PRIVATE erbsland::core)
target_compile_options(x509-der-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(x509-der-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
erbsland_set_required_compiler_options(x509-der-fuzz)

add_subdirectory(src)
