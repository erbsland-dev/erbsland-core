# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        AesBlockCipher.hpp
        AesBlockCipherFactory.cpp
        AesBlockCipherFactory.hpp
        AesKeySchedule.cpp
        AesKeySchedule.hpp
        AesOperations.cpp
        AesOperations.hpp
        GaloisMultiplier.hpp
        GaloisMultiplierFactory.cpp
        GaloisMultiplierFactory.hpp
        GaloisOperations.cpp
        GaloisOperations.hpp
        GHash.cpp
        GHash.hpp
        HardwareFeatures.cpp
        HardwareFeatures.hpp
        PortableAesBlockCipher.cpp
        PortableAesBlockCipher.hpp
        PortableGaloisMultiplier.cpp
        PortableGaloisMultiplier.hpp
)

# Native feature discovery includes platform headers and must not leak their macros into a unity batch.
set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/HardwareFeatures.cpp"
        TARGET_DIRECTORY erbsland-core
        PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE
)

if (CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" _erbsland_aes_processor)
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _erbsland_aes_processor)
endif()

if (_erbsland_aes_processor MATCHES "^(arm64|aarch64)$")
    target_compile_definitions(erbsland-core PRIVATE ERBSLAND_AES_ARM_BACKEND=1)
    target_sources(erbsland-core PRIVATE
            ArmAesBlockCipher.cpp
            ArmAesBlockCipher.hpp
            ArmGaloisMultiplier.cpp
            ArmGaloisMultiplier.hpp
    )
    set_source_files_properties(
            "${CMAKE_CURRENT_SOURCE_DIR}/ArmAesBlockCipher.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/ArmGaloisMultiplier.cpp"
            TARGET_DIRECTORY erbsland-core
            PROPERTIES
                SKIP_PRECOMPILE_HEADERS TRUE
                SKIP_UNITY_BUILD_INCLUSION TRUE
    )
    if (NOT MSVC AND NOT APPLE)
        set_source_files_properties(
                "${CMAKE_CURRENT_SOURCE_DIR}/ArmAesBlockCipher.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/ArmGaloisMultiplier.cpp"
                TARGET_DIRECTORY erbsland-core
                PROPERTIES COMPILE_OPTIONS "-march=armv8-a+crypto"
        )
    endif()
elseif (_erbsland_aes_processor MATCHES "^(amd64|x64|x86_64)$")
    target_compile_definitions(erbsland-core PRIVATE ERBSLAND_AES_X86_BACKEND=1)
    target_sources(erbsland-core PRIVATE
            X86AesBlockCipher.cpp
            X86AesBlockCipher.hpp
            X86GaloisMultiplier.cpp
            X86GaloisMultiplier.hpp
    )
    set_source_files_properties(
            "${CMAKE_CURRENT_SOURCE_DIR}/X86AesBlockCipher.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/X86GaloisMultiplier.cpp"
            TARGET_DIRECTORY erbsland-core
            PROPERTIES
                SKIP_PRECOMPILE_HEADERS TRUE
                SKIP_UNITY_BUILD_INCLUSION TRUE
    )
    if (NOT MSVC)
        set_source_files_properties(
                "${CMAKE_CURRENT_SOURCE_DIR}/X86AesBlockCipher.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/X86GaloisMultiplier.cpp"
                TARGET_DIRECTORY erbsland-core
                PROPERTIES COMPILE_OPTIONS "-maes;-mpclmul"
        )
    endif()
endif()
