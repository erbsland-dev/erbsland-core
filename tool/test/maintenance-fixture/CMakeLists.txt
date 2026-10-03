# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(erbsland-core-tool-fake-git)
erbsland_core_setup_application(TARGET erbsland-core-tool-fake-git)
add_executable(erbsland-core-tool-fake-zizmor)
erbsland_core_setup_application(TARGET erbsland-core-tool-fake-zizmor)
add_subdirectory(src)
