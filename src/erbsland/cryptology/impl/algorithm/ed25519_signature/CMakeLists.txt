# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Ed25519Signature.cpp
        Ed25519Signature.hpp
        Ed25519SignatureTypes.hpp
        Ed25519Signer.cpp
        Ed25519Signer.hpp
        FieldElement.cpp
        FieldElement.hpp
)
