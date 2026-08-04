# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(pkcs8-key-fuzz)
target_sources(pkcs8-key-fuzz PRIVATE Pkcs8KeyFuzz.cpp)
target_link_libraries(pkcs8-key-fuzz PRIVATE erbsland::core)
target_compile_options(pkcs8-key-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(pkcs8-key-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
erbsland_set_required_compiler_options(pkcs8-key-fuzz)
