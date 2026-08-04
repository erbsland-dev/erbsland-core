# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(asn1)
add_subdirectory(configuration)
add_subdirectory(impl)
add_subdirectory(keys)
add_subdirectory(protected_data)
add_subdirectory(symmetric)
add_subdirectory(tls)
add_subdirectory(tls_record)
add_subdirectory(unsafe)
add_subdirectory(x509)

target_sources(erbsland-core PRIVATE
        CryptographicSecurity.hpp
        CryptographicStatus.hpp
        CryptologyError.hpp
        HashAlgorithm.cpp
        HashAlgorithm.hpp
        Hasher.cpp
        Hasher.hpp
        HashRequirements.hpp
        HashSelector.cpp
        HashSelector.hpp
        HashSelector_fwd.hpp
        HashThroughput.hpp
        Hkdf.cpp
        Hkdf.hpp
        Hkdf_fwd.hpp
        Hmac.cpp
        Hmac.hpp
        PasswordHash.cpp
        PasswordHash.hpp
        PasswordHashAlgorithm.cpp
        PasswordHashAlgorithm.hpp
        PasswordHasher.cpp
        PasswordHasher.hpp
        PasswordHasher_fwd.hpp
        PasswordHashKey.cpp
        PasswordHashKey.hpp
        PasswordHashPolicy.cpp
        PasswordHashPolicy.hpp
        PasswordHashPolicy_fwd.hpp
        PasswordVerification.cpp
        PasswordVerification.hpp
        StdFormat.hpp
)
