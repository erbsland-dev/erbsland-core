# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)
add_subdirectory(unsafe)

target_sources(erbsland-core PRIVATE
        CryptographicSecurity.hpp
        CryptographicStatus.hpp
        HashAlgorithm.cpp
        HashAlgorithm.hpp
        Hasher.cpp
        Hasher.hpp
        HashRequirements.hpp
        HashThroughput.hpp
        PasswordHash.cpp
        PasswordHash.hpp
        PasswordHashAlgorithm.cpp
        PasswordHashAlgorithm.hpp
        PasswordHasher.cpp
        PasswordHasher.hpp
        PasswordHashKey.cpp
        PasswordHashKey.hpp
        PasswordHashPolicy.cpp
        PasswordHashPolicy.hpp
        PasswordVerification.cpp
        PasswordVerification.hpp
        StdFormat.hpp
)
