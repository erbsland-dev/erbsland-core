# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

add_executable(erbsland-core-tool-unittest)
target_link_libraries(erbsland-core-tool-unittest PRIVATE erbsland-core-tool-lib)
add_subdirectory(src)
erbsland_core_configure_unittest(erbsland-core-tool-unittest)
add_test(NAME erbsland-core-tool-unittest COMMAND $<TARGET_FILE:erbsland-core-tool-unittest> --no-color)
set_tests_properties(erbsland-core-tool-unittest PROPERTIES LABELS ci)

add_dependencies(erbsland-core-tool-unittest erbsland-core-tool-fake-sphinx)
target_compile_definitions(erbsland-core-tool-unittest PRIVATE
        ERBSLAND_CORE_TOOL_FAKE_SPHINX="$<TARGET_FILE:erbsland-core-tool-fake-sphinx>")

add_dependencies(erbsland-core-tool-unittest erbsland-core-tool-fake-clang-format)
target_compile_definitions(erbsland-core-tool-unittest PRIVATE
        ERBSLAND_CORE_TOOL_FAKE_CLANG_FORMAT="$<TARGET_FILE:erbsland-core-tool-fake-clang-format>")

add_dependencies(erbsland-core-tool-unittest erbsland-core-tool-capture-fixture)
target_compile_definitions(erbsland-core-tool-unittest PRIVATE
        ERBSLAND_CORE_TOOL_CAPTURE_FIXTURE="$<TARGET_FILE:erbsland-core-tool-capture-fixture>")

add_dependencies(erbsland-core-tool-unittest erbsland-core-tool-fake-git erbsland-core-tool-fake-zizmor)
target_compile_definitions(erbsland-core-tool-unittest PRIVATE
        ERBSLAND_CORE_TOOL_FAKE_GIT="$<TARGET_FILE:erbsland-core-tool-fake-git>"
        ERBSLAND_CORE_TOOL_FAKE_ZIZMOR="$<TARGET_FILE:erbsland-core-tool-fake-zizmor>")
