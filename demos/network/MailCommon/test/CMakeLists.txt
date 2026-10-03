# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-mail-unittest)
add_subdirectory(src)
target_link_libraries(erbsland-core-mail-unittest PRIVATE erbsland-core-mail-support)
target_include_directories(erbsland-core-mail-unittest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../MailReceiver/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../MailSender/src"
)
erbsland_set_required_compiler_options(erbsland-core-mail-unittest)
erbsland_core_configure_unittest(erbsland-core-mail-unittest ENABLE_WARNINGS)
add_test(NAME erbsland-core-mail-unittest COMMAND erbsland-core-mail-unittest --no-color)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME mail-tls-integration COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration.py" "${CMAKE_BINARY_DIR}/demo-apps/network")
set_tests_properties(mail-tls-integration PROPERTIES TIMEOUT 60)
