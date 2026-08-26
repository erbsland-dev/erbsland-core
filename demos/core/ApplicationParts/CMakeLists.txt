# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(core_detached_parts)
target_sources(core_detached_parts PRIVATE
        CatalogServer.hpp
        CatalogServerPart.hpp
        CatalogStorage.hpp
        CatalogStoragePart.hpp
        DetachedMain.cpp
)

erbsland_core_add_demo(core_part_application)
target_sources(core_part_application PRIVATE
        ApplicationMain.cpp
        CatalogServer.hpp
        CatalogServerPart.hpp
        CatalogStorage.hpp
        CatalogStoragePart.hpp
)
