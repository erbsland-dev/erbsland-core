# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        KeyAgreementAlgorithm.cpp
        KeyAgreementAlgorithm.hpp
        KeyAgreementPrivateKey.cpp
        KeyAgreementPrivateKey.hpp
        KeyAgreementPrivateKey_fwd.hpp
        KeyAgreementPublicKey.cpp
        KeyAgreementPublicKey.hpp
        KeyAgreementSharedSecret.cpp
        KeyAgreementSharedSecret.hpp
        KeyAgreementSharedSecret_fwd.hpp
        PublicKey.cpp
        PublicKey.hpp
        PublicKey_fwd.hpp
        SigningKeyAlgorithm.hpp
        SigningKeyProfile.hpp
        SigningPrivateKey.cpp
        SigningPrivateKey.hpp
        SigningPrivateKey_fwd.hpp
)
