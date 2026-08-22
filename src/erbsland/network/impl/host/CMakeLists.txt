# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        CommonHostTests.cpp
        CommonHostTests.hpp
        HostLookup.cpp
        HostLookup.hpp
        HostLookup_fwd.hpp
        HostLookupEventEditor.cpp
        HostLookupEventEditor.hpp
        HostLookupOperation.cpp
        HostLookupOperation.hpp
        HostLookupOperation_fwd.hpp
        HostResolver.hpp
        HostResolver_fwd.hpp
        HostResolverErrorContext.cpp
        HostResolverErrorContext.hpp
)

if (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsHostResolver.cpp
            WindowsHostResolver.hpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            PosixHostResolver.cpp
            PosixHostResolver.hpp
    )
endif ()
