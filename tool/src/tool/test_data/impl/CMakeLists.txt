# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-lib PRIVATE
        CalendarDate.cpp
        CalendarDate.hpp
        CalendarDateTime.cpp
        CalendarDateTime.hpp
        CalendarTime.cpp
        CalendarTime.hpp
        CategoryTestData.cpp
        CategoryTestData.hpp
        SaturatingMathData.cpp
        SaturatingMathData.hpp
        VectorConverter.cpp
        VectorConverter.hpp
        VectorConverter_archive.cpp
        VectorConverter_json.cpp
        VectorConverter_wycheproof.cpp
        VectorDownloader.cpp
        VectorDownloader.hpp
        VectorSource.cpp
        VectorSource.hpp
)
