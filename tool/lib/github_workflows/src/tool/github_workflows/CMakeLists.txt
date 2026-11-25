# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-github_workflows PRIVATE
        ActionReference.hpp
        ActionTag.hpp
        GitHubWorkflowsTool.cpp
        GitHubWorkflowsTool.hpp
        GitTagCache.cpp
        GitTagCache.hpp
        StableTag.cpp
        StableTag.hpp
        TagCatalog.cpp
        TagCatalog.hpp
        WorkflowFile.cpp
        WorkflowFile.hpp
        WorkflowLine.hpp
)
