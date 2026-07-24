# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BackendFactory.cpp
        BackendFactory.hpp
        CharactersSets.cpp
        CharactersSets.hpp
        CommonPathBackend.cpp
        CommonPathBackend.hpp
        PathBackend.cpp
        PathBackend.hpp
        PathConstants.hpp
        PathContent.cpp
        PathContent.hpp
        PathContent_fwd.hpp
        PathData.cpp
        PathData.hpp
        PathErrorDiagnostic.cpp
        PathErrorDiagnostic.hpp
        PathInfoCache.hpp
        PathInfoCache_fwd.hpp
        PathInfoData.cpp
        PathInfoData.hpp
        PathNameTools.cpp
        PathNameTools.hpp
        PathOperations.cpp
        PathOperations.hpp
        PathOperations_fwd.hpp
        PathParser.cpp
        PathParser.hpp
        PathWalker.cpp
        PathWalker.hpp
        PathWalker_fwd.hpp
)

if (WIN32)
    target_link_libraries(erbsland-core PRIVATE Userenv)
    target_sources(erbsland-core PRIVATE
            WindowsAccessProfileSecurity.cpp
            WindowsAccessProfileSecurity.hpp
            WindowsPathBackend.cpp
            WindowsPathBackend.hpp
            WindowsPathBackend_operations.cpp
            WindowsPathBackend_path.cpp
    )
else ()
    target_sources(erbsland-core PRIVATE
            PosixPathBackend.cpp
            PosixPathBackend.hpp
            PosixPathBackend_operations.cpp
            PosixPathBackend_path.cpp
    )
endif ()
