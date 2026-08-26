# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        Context.hpp
        Context_fwd.hpp
        Delimiters.hpp
        Environment.cpp
        Environment.hpp
        Environment_fwd.hpp
        EnvironmentOptions.cpp
        EnvironmentOptions.hpp
        FileSystemLoader.cpp
        FileSystemLoader.hpp
        FileSystemLoaderOptions.hpp
        LayoutSource.hpp
        Loader.hpp
        Loader_fwd.hpp
        RenderError.cpp
        RenderError.hpp
        RenderErrorCategory.hpp
        RenderErrorContext.hpp
        RenderLimits.cpp
        RenderLimits.hpp
        ResourceLoader.cpp
        ResourceLoader.hpp
        Value.cpp
        Value.hpp
        Value_fwd.hpp
        ValueType.hpp
)
