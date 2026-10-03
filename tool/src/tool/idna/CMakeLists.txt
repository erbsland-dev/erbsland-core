# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-lib PRIVATE
        IdnaAttributes.hpp
        IdnaBidi.hpp
        IdnaData.cpp
        IdnaData.hpp
        IdnaJoining.hpp
        IdnaRange.hpp
        IdnaScript.hpp
        IdnaStatus.hpp
        IdnaTool.cpp
        IdnaTool.hpp
        LabelProcessor.cpp
        LabelProcessor.hpp
)
