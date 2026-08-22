# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        HttpCookieJar.cpp
        HttpCookieJar.hpp
        HttpCookieJarEntry.hpp
        HttpCookieSessionManagerData.cpp
        HttpCookieSessionManagerData.hpp
        HttpCookieSessionManagerData_fwd.hpp
        PublicSuffixData.cpp
        PublicSuffixData.hpp
        PublicSuffixList.cpp
        PublicSuffixList.hpp
        PublicSuffixLookup.cpp
        PublicSuffixLookup.hpp
)
