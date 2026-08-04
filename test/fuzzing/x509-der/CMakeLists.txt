# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(x509-der-fuzz)
target_sources(x509-der-fuzz PRIVATE X509DerFuzz.cpp)
target_link_libraries(x509-der-fuzz PRIVATE erbsland::core)
target_compile_options(x509-der-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(x509-der-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
erbsland_set_required_compiler_options(x509-der-fuzz)
