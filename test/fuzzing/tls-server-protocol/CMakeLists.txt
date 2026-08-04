# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(tls-server-protocol-fuzz)
target_sources(tls-server-protocol-fuzz PRIVATE TlsServerProtocolFuzz.cpp)
target_link_libraries(tls-server-protocol-fuzz PRIVATE erbsland::core)
target_include_directories(tls-server-protocol-fuzz PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(tls-server-protocol-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(tls-server-protocol-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
erbsland_set_required_compiler_options(tls-server-protocol-fuzz)
