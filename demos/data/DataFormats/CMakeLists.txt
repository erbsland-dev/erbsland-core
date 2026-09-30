# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(data_formats)
target_sources(data_formats PRIVATE
        BsonExamples.cpp
        CborExamples.cpp
        DataFormatsDemos.hpp
        TempoVariations.cpp
        XmlExamples.cpp
        main.cpp
)
