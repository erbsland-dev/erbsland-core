# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_library(erbsland-core-stream-demo-support STATIC)
target_sources(erbsland-core-stream-demo-support PRIVATE
        ScriptedByteInputStream.cpp
        ScriptedByteInputStream.hpp
        ScriptedByteOutputStream.cpp
        ScriptedByteOutputStream.hpp
        StreamDemoSupport.cpp
        StreamDemoSupport.hpp
)
target_link_libraries(erbsland-core-stream-demo-support PUBLIC erbsland::core)
target_include_directories(erbsland-core-stream-demo-support
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../_common/src
)
erbsland_set_required_compiler_options(erbsland-core-stream-demo-support)
erbsland_enable_debug_warnings(erbsland-core-stream-demo-support)
set_target_properties(erbsland-core-stream-demo-support PROPERTIES FOLDER "Demos/stream")
