# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(string_char_reader)
target_sources(string_char_reader PRIVATE
        main.cpp
        StringCharReaderDemos.hpp
        StudyPlanParser.cpp
)
