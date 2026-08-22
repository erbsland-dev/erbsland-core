# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        UrlData.hpp
        UrlData_fwd.hpp
        UrlParser.cpp
        UrlParser.hpp
        UrlResolver.cpp
        UrlResolver.hpp
        UrlWriter.cpp
        UrlWriter.hpp
)
