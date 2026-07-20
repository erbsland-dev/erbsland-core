# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        IsoDateTimeParser.cpp
        IsoDateTimeParser.hpp
        ParsedIsoDateTime.hpp
        PosixTimeConverter.cpp
        PosixTimeConverter.hpp
        TimeDeltaFormatter.cpp
        TimeDeltaFormatter.hpp
        TimePart.hpp
        TimePartBases.hpp
        TimePartWithAmount.hpp
        WindowsTimeConverter.cpp
        WindowsTimeConverter.hpp
)
