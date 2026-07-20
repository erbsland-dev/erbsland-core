# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Attribute.cpp
        Attribute.hpp
        CaseSensitive.cpp
        CaseSensitive.hpp
        ConfVersion.cpp
        ConfVersion.hpp
        CustomError.cpp
        CustomError.hpp
        Default.cpp
        Default.hpp
        Dependency.cpp
        Dependency.hpp
        Description.cpp
        Description.hpp
        IsOptional.cpp
        IsOptional.hpp
        IsSecret.cpp
        IsSecret.hpp
        KeyIndex.cpp
        KeyIndex.hpp
        MaximumVersion.cpp
        MaximumVersion.hpp
        MinimumVersion.cpp
        MinimumVersion.hpp
        Title.cpp
        Title.hpp
        Type.cpp
        Type.hpp
)
