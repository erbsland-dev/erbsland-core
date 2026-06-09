# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ApiWrapperDispatchTest.cpp
        BackendTest.cpp
        InputDispatchProbe.hpp
        InputTest.cpp
        InputTestProbe.hpp
        KeyModifiersTest.cpp
        KeysTest.cpp
        KeyTest.cpp
        ReadableBufferDispatchProbe.hpp
        TerminalConvenienceTest.cpp
        TerminalFlagsTest.cpp
        TerminalOptionsRendererTest.cpp
        TerminalStreamTest.cpp
        TerminalTest.cpp
        WritableBufferDispatchProbe.hpp
)
