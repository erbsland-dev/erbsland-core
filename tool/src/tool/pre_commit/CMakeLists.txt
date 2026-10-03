# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core-tool-lib PRIVATE
        CaptureGitStateTask.cpp
        CaptureGitStateTask.hpp
        GitState.cpp
        GitState.hpp
        GitStateSnapshot.hpp
        PreCommitRunner.cpp
        PreCommitRunner.hpp
        PreCommitTool.cpp
        PreCommitTool.hpp
        VerifyGitStateTask.cpp
        VerifyGitStateTask.hpp
)
