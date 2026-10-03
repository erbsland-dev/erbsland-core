# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(erbsland-core-tool-fake-sphinx)
target_compile_features(erbsland-core-tool-fake-sphinx PRIVATE cxx_std_20)

add_subdirectory(src)
