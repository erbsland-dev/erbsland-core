# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(erbsland-core-tool-capture-fixture)
target_compile_features(erbsland-core-tool-capture-fixture PRIVATE cxx_std_20)
target_link_libraries(erbsland-core-tool-capture-fixture PRIVATE erbsland::core)
add_subdirectory(src)
