# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

if(TARGET erbsland-core-namespace-consumer)
    target_sources(erbsland-core-namespace-consumer PRIVATE
            flattened.cpp
            leaf.cpp
    )

    target_sources(erbsland-core-namespace-explicit-consumer PRIVATE
            explicit.cpp
    )

    target_sources(erbsland-core-namespace-library PRIVATE
            explicit.cpp
    )
else()
    target_sources(installed-namespace-consumer PRIVATE
            flattened.cpp
    )

    target_sources(installed-namespace-explicit-consumer PRIVATE
            explicit.cpp
    )
endif()
