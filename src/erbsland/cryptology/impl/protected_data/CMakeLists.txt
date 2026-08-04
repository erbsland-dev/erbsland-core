# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ApplicationProtectedDataAccess.cpp
        ApplicationProtectedDataAccess.hpp
        InternalProtectedDataProvider.cpp
        InternalProtectedDataProvider.hpp
        ProtectedDataAccess.cpp
        ProtectedDataAccess.hpp
        ProtectedDataAccess_fwd.hpp
        ProtectedDataProvider.hpp
        ProtectedDataProvider_fwd.hpp
        ProtectedDataProvider_factory.cpp
)

if (APPLE)
    target_sources(erbsland-core PRIVATE
            MacosDataReference.hpp
            MacosProtectedDataProvider.cpp
            MacosProtectedDataProvider.hpp
    )
    target_link_libraries(erbsland-core PRIVATE "-framework Security" "-framework CoreFoundation")
elseif (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsProtectedDataApi.cpp
            WindowsProtectedDataApi.hpp
            WindowsLocalBuffer.cpp
            WindowsLocalBuffer.hpp
            WindowsProtectedDataProvider.cpp
            WindowsProtectedDataProvider.hpp
    )
endif ()
