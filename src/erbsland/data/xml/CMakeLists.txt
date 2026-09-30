# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        XmlAttribute.hpp
        XmlDocument.cpp
        XmlDocument.hpp
        XmlFormatOptions.hpp
        XmlNode.cpp
        XmlNode.hpp
        XmlNode_fwd.hpp
        XmlNodeType.hpp
        XmlParseOptions.hpp
)
