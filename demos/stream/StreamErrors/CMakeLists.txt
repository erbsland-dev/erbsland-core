# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(stream_errors)
target_sources(stream_errors PRIVATE
        DistinguishResultStates.cpp
        HandleEncodingError.cpp
        InspectStreamError.cpp
        main.cpp
        PreserveWrappedContext.cpp
        ReportDiagnostic.cpp
        StreamErrorsDemos.hpp
)
