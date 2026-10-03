# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-lib PRIVATE
        TimeZoneDataTool.cpp
        TimeZoneDataTool.hpp
        TimeZoneModel.cpp
        TimeZoneModel.hpp
        TimeZoneModel_calendar.cpp
        TimeZoneRenderer.cpp
        TimeZoneRenderer.hpp
        TimeZoneRenderer_metadata.cpp
        TimeZoneRenderer_zones.cpp
        TimeZoneSource.cpp
        TimeZoneSource.hpp
)
