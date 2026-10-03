# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(package_client PRIVATE
        main.cpp
)

target_sources(package_server PRIVATE
        main.cpp
)

target_sources(package-library PRIVATE
        library.cpp
)

if(APPLE)
    target_sources(PackageFramework PRIVATE
            framework.cpp
    )
endif()

if(WIN32)
    target_sources(package-signing-tool PRIVATE
            signing-tool.cpp
    )
endif()
