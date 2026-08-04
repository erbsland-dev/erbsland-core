# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(tls-record-fuzz)
target_sources(tls-record-fuzz PRIVATE TlsRecordFuzz.cpp)
target_link_libraries(tls-record-fuzz PRIVATE erbsland::core)
target_compile_options(tls-record-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(tls-record-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
erbsland_set_required_compiler_options(tls-record-fuzz)
