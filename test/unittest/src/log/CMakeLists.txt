# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ActiveLogConfigurationTest.cpp
        ConsoleLogWriterTest.cpp
        LogConfigurationParserTest.cpp
        LogCoreTest.cpp
        LogLineFormatterTest.cpp
        LogRouteFormatTest.cpp
        LogTraceSectionTest.cpp
        LogWriterFilterTest.cpp
        LogWriterTest.cpp
        SyslogLogWriterTest.cpp
)
