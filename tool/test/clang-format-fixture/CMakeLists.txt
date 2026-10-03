# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(erbsland-core-tool-fake-clang-format)
erbsland_core_setup_application(TARGET erbsland-core-tool-fake-clang-format)

add_subdirectory(src)
