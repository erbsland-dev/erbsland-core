# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core-tool-frame PRIVATE
        CommandOutput.hpp
        DataSource.cpp
        DataSource.hpp
        ExternalCommand.cpp
        ExternalCommand.hpp
        PreCommitContext.hpp
        PreCommitTask.cpp
        PreCommitTask.hpp
        ProjectAccessCheck.cpp
        ProjectAccessCheck.hpp
        ProjectFiles.cpp
        ProjectFiles.hpp
        ProjectRoot.cpp
        ProjectRoot.hpp
        ProjectTextFile.cpp
        ProjectTextFile.hpp
        TaskProgress.cpp
        TaskProgress.hpp
        TaskProgressSnapshot.hpp
        TemplateLanguage.hpp
        TextChecksum.cpp
        TextChecksum.hpp
        Tool.cpp
        Tool.hpp
        ToolApplication.cpp
        ToolApplication.hpp
        ToolApplication_fwd.hpp
        ToolApplication_sources.cpp
        ToolApplication_tasks.cpp
        ToolProject.hpp
)
