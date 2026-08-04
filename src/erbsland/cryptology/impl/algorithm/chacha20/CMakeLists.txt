# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        ChaCha20Backend.hpp
        ChaCha20BackendFactory.cpp
        ChaCha20BackendFactory.hpp
        ChaCha20Operations.cpp
        ChaCha20Operations.hpp
        Poly1305.hpp
        Poly1305Factory.cpp
        Poly1305Factory.hpp
        PortableChaCha20Backend.cpp
        PortableChaCha20Backend.hpp
        PortablePoly1305.cpp
        PortablePoly1305.hpp
)

if (CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" _erbsland_chacha20_processor)
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _erbsland_chacha20_processor)
endif()

if (_erbsland_chacha20_processor MATCHES "^(arm64|aarch64)$")
    target_compile_definitions(erbsland-core PRIVATE ERBSLAND_CHACHA20_ARM_BACKEND=1)
    target_sources(erbsland-core PRIVATE
            ArmChaCha20Backend.cpp
            ArmChaCha20Backend.hpp
            ArmPoly1305.cpp
            ArmPoly1305.hpp
    )
    set_source_files_properties(
            "${CMAKE_CURRENT_SOURCE_DIR}/ArmChaCha20Backend.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/ArmPoly1305.cpp"
            TARGET_DIRECTORY erbsland-core
            PROPERTIES
                SKIP_PRECOMPILE_HEADERS TRUE
                SKIP_UNITY_BUILD_INCLUSION TRUE
    )
elseif (_erbsland_chacha20_processor MATCHES "^(amd64|x64|x86_64)$")
    target_compile_definitions(erbsland-core PRIVATE ERBSLAND_CHACHA20_X86_BACKEND=1)
    target_sources(erbsland-core PRIVATE
            X86ChaCha20Backend.cpp
            X86ChaCha20Backend.hpp
            X86Poly1305.cpp
            X86Poly1305.hpp
    )
    set_source_files_properties(
            "${CMAKE_CURRENT_SOURCE_DIR}/X86ChaCha20Backend.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/X86Poly1305.cpp"
            TARGET_DIRECTORY erbsland-core
            PROPERTIES
                SKIP_PRECOMPILE_HEADERS TRUE
                SKIP_UNITY_BUILD_INCLUSION TRUE
    )
    if (NOT MSVC)
        set_source_files_properties(
                "${CMAKE_CURRENT_SOURCE_DIR}/X86ChaCha20Backend.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/X86Poly1305.cpp"
                TARGET_DIRECTORY erbsland-core
                PROPERTIES COMPILE_OPTIONS "-msse2"
        )
    endif()
endif()
