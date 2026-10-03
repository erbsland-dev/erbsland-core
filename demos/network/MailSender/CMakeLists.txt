# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(mail_sender)
target_link_libraries(mail_sender PRIVATE erbsland-core-mail-support)
add_subdirectory(src)
file(GLOB_RECURSE _mailAssets CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/conf" "conf/*")
foreach(_asset IN LISTS _mailAssets)
    configure_file("conf/${_asset}" "${CMAKE_BINARY_DIR}/demo-apps/network/${_asset}" COPYONLY)
endforeach()
