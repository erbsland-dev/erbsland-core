# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(caption-player PRIVATE
        CaptionPlayerApp.cpp
        CaptionPlayerApp.hpp
        main.cpp
)
target_sources(erbsland-core-caption-player-support PRIVATE
        CaptionBackend.cpp
        CaptionBackend.hpp
        CaptionCue.hpp
        CaptionProtocol.hpp
        CaptionTrack.cpp
        CaptionTrack.hpp
        CueBoundary.hpp
        CueEventData.hpp
)
