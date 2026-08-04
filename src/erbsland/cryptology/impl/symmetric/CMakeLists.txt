# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        AesCbcDecryptorData.cpp
        AesCbcDecryptorData.hpp
        AesCbcEncryptorData.cpp
        AesCbcEncryptorData.hpp
        AesCbcState.cpp
        AesCbcState.hpp
        AesGcmDecryptorData.cpp
        AesGcmDecryptorData.hpp
        AesGcmEncryptorData.cpp
        AesGcmEncryptorData.hpp
        AesGcmState.cpp
        AesGcmState.hpp
        ChaCha20Poly1305DecryptorData.cpp
        ChaCha20Poly1305DecryptorData.hpp
        ChaCha20Poly1305EncryptorData.cpp
        ChaCha20Poly1305EncryptorData.hpp
        ChaCha20Poly1305State.cpp
        ChaCha20Poly1305State.hpp
        SymmetricDataFactory.cpp
        SymmetricDataFactory.hpp
        SymmetricDecryptorBackendAccess.hpp
        SymmetricDecryptorBackendAccess_fwd.hpp
        SymmetricDecryptorData.hpp
        SymmetricDecryptorData_fwd.hpp
        SymmetricEncryptorBackendAccess.hpp
        SymmetricEncryptorBackendAccess_fwd.hpp
        SymmetricEncryptorData.hpp
        SymmetricEncryptorData_fwd.hpp
        UnsafeSymmetricKeyAccess.hpp
        UnsafeSymmetricKeyAccess_fwd.hpp
)
