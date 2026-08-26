# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(aes)
add_subdirectory(chacha20)
add_subdirectory(ecdsa_signature)
add_subdirectory(ed25519_signature)
add_subdirectory(rsa_signature)
add_subdirectory(x25519)

target_sources(erbsland-core PRIVATE
        Argon2id.hpp
        Blake2b.hpp
        HmacAlgorithm.hpp
        HmacSha256.hpp
        Keccak.hpp
        Md5.hpp
        NistPrimeCurve.cpp
        NistPrimeCurve.hpp
        NistPrimeCurve_arithmetic.cpp
        Scrypt.hpp
        Sha1.hpp
        Sha2.hpp
        Sha3.hpp
)
