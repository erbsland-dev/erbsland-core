# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        BsonValueTest.cpp
        CborValueTest.cpp
        JsonParserTest.cpp
        JsonValueTest.cpp
        XmlDocumentTest.cpp
)
