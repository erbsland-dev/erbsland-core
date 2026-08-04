# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        PrivateKeyData.cpp
        RsaMontgomeryScratch.hpp
        RsaSignature.cpp
        RsaSignature.hpp
        RsaSignature_arithmetic.cpp
        RsaSignature_decode.cpp
        RsaSignatureTypes.hpp
        RsaSigner.cpp
        RsaSigner.hpp
        RsaSigner_arithmetic.cpp
        RsaSigner_decode.cpp
)
