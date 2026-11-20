# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(pipe-log-summary PRIVATE
        LogCounts.cpp
        LogCounts.hpp
        PipeBackend.cpp
        PipeBackend.hpp
        PipeLogSummaryApp.cpp
        PipeLogSummaryApp.hpp
        main.cpp
)
target_sources(erbsland-core-pipe-log-summary-support PRIVATE
        PosixPipeDriver.cpp
        PosixPipeDriver.hpp
)
