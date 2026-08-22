# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        ResourceError.cpp
        ResourceError.hpp
        ResourceErrorCategory.hpp
        ResourceInfo.cpp
        ResourceInfo.hpp
        ResourceInfo_fwd.hpp
        ResourceManager.cpp
        ResourceManager.hpp
        ResourceManager_fwd.hpp
        Resources.hpp
        Resources_fwd.hpp
        ResourceStorageRegistration.hpp
)
