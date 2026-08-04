# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BooleanValue.hpp
        BytesValue.cpp
        BytesValue.hpp
        CalendarDeltaValue.hpp
        Container.hpp
        Container_fwd.hpp
        DateTimeValue.hpp
        DateValue.hpp
        DirectStorageAccess.hpp
        Document.cpp
        Document.hpp
        DocumentBuilder.cpp
        DocumentBuilder.hpp
        DocumentBuilderStorage.cpp
        DocumentBuilderStorage.hpp
        FloatValue.hpp
        IntegerValue.hpp
        IntermediateSection.hpp
        RegExValue.hpp
        Section.hpp
        SectionList.hpp
        SectionWithNames.hpp
        SectionWithTexts.hpp
        TextValue.hpp
        TimeValue.hpp
        TimeWithZoneValue.hpp
        Value.cpp
        Value.hpp
        Value_fwd.hpp
        ValueHelper.hpp
        ValueList.hpp
        ValueMap.cpp
        ValueMap.hpp
        ValueTreeHelper.cpp
        ValueTreeHelper.hpp
        ValueTreeWalker.cpp
        ValueTreeWalker.hpp
        ValueWithChildren.hpp
        ValueWithConvertibleType.hpp
        ValueWithNativeType.hpp
)
