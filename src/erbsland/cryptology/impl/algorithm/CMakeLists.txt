# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Argon2id.hpp
        Blake2b.hpp
        HmacSha256.hpp
        Keccak.hpp
        Md5.hpp
        Scrypt.hpp
        Sha1.hpp
        Sha2.hpp
        Sha3.hpp
)
