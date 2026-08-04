# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(ecdsa-signature-fuzz)
target_sources(ecdsa-signature-fuzz PRIVATE EcdsaSignatureFuzz.cpp)
target_link_libraries(ecdsa-signature-fuzz PRIVATE erbsland::core)
target_compile_options(ecdsa-signature-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_options(ecdsa-signature-fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
erbsland_set_required_compiler_options(ecdsa-signature-fuzz)
