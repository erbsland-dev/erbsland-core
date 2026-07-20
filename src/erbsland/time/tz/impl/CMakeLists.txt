# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        AbbreviationListOffset.hpp
        AbbreviationOffset.hpp
        ContinuationLine.hpp
        Database.cpp
        Database.hpp
        Database_fwd.hpp
        Database_version.cpp
        DayRule.hpp
        Info.cpp
        Info.hpp
        LocalTimeZoneBackend.cpp
        LocalTimeZoneBackend.hpp
        LocalTimeZoneBackend_fwd.hpp
        RegionAfrica.cpp
        RegionAfrica.hpp
        RegionAntarctica.cpp
        RegionAntarctica.hpp
        RegionAsia.cpp
        RegionAsia.hpp
        RegionAustralasia.cpp
        RegionAustralasia.hpp
        RegionEtcetera.cpp
        RegionEtcetera.hpp
        RegionEurope.cpp
        RegionEurope.hpp
        RegionNorthamerica.cpp
        RegionNorthamerica.hpp
        RegionSouthamerica.cpp
        RegionSouthamerica.hpp
        Rule.hpp
        RuleAtTimeZone.hpp
        Rules.cpp
        Rules.hpp
        RuleSet.hpp
        RuleSetId.hpp
        Text.hpp
        TextId.hpp
        Texts.cpp
        Texts.hpp
        TimeReference.hpp
        ZoneId.hpp
        ZoneName.hpp
        Zones.cpp
        Zones.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsLocalTimeZoneBackend.cpp
            WindowsLocalTimeZoneBackend.hpp
            WindowsTimeZoneMap.cpp
            WindowsTimeZoneMap.hpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            PosixLocalTimeZoneBackend.cpp
            PosixLocalTimeZoneBackend.hpp
    )
endif ()
