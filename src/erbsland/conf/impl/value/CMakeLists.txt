# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BytesValue.cpp
        BytesValue.hpp
        Container.hpp
        DirectStorageAccess.hpp
        Document.cpp
        Document.hpp
        DocumentBuilder.cpp
        DocumentBuilder.hpp
        DocumentBuilderStorage.cpp
        DocumentBuilderStorage.hpp
        Section.hpp
        SectionList.hpp
        Value.cpp
        Value.hpp
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
