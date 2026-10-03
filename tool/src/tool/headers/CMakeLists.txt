# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core-tool-lib PRIVATE
        DirectoryPolicy.hpp
        HeaderDeclaration.hpp
        HeaderDeclarations.cpp
        HeaderDeclarations.hpp
        HeaderOutput.hpp
        HeaderPolicy.cpp
        HeaderPolicy.hpp
        HeaderPolicyConfig.hpp
        HeaderReadResult.hpp
        HeaderRenderJob.hpp
        PublicHeader.hpp
        PublicHeaderMap.cpp
        PublicHeaderMap.hpp
        PublicHeaders.cpp
        PublicHeaders.hpp
        PublicHeaders_files.cpp
        PublicHeaders_render.cpp
        PublicHeadersTool.cpp
        PublicHeadersTool.hpp
)
