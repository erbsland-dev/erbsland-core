# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-pi-calculator-unittest)
add_subdirectory(src)
target_link_libraries(erbsland-core-pi-calculator-unittest PRIVATE erbsland-core-pi-calculator-support)
erbsland_set_required_compiler_options(erbsland-core-pi-calculator-unittest)
erbsland_core_configure_unittest(erbsland-core-pi-calculator-unittest ENABLE_WARNINGS)
set_target_properties(erbsland-core-pi-calculator-unittest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demos/event/PiCalculator/test"
)

add_test(NAME erbsland-core-pi-calculator-unittest COMMAND erbsland-core-pi-calculator-unittest --no-color)
add_test(NAME pi-calculator-finite COMMAND $<TARGET_FILE:pi_calculator> --plain --digits 100)
set_tests_properties(pi-calculator-finite PROPERTIES
        PASS_REGULAR_EXPRESSION "3\.14159265358979323846264338327950288419716939937510"
        TIMEOUT 5
)
