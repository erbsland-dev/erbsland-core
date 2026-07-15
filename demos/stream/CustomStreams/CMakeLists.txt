# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(custom_streams)
target_sources(custom_streams PRIVATE
        CustomStreamsDemos.hpp
        main.cpp
        ReadCustomStream.cpp
        SkillTreeStreams.cpp
        SkillTreeStreams.hpp
        UseCustomStreamAsynchronously.cpp
        VerifyCustomContracts.cpp
        WriteCustomStream.cpp
)
