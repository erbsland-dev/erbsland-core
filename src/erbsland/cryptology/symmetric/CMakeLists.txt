# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        CryptographicDataBlock.cpp
        CryptographicDataBlock.hpp
        SymmetricCipher.hpp
        SymmetricDecryptor.cpp
        SymmetricDecryptor.hpp
        SymmetricEncryptionRequirements.hpp
        SymmetricEncryptionSelector.cpp
        SymmetricEncryptionSelector.hpp
        SymmetricEncryptionSelector_fwd.hpp
        SymmetricEncryptionType.cpp
        SymmetricEncryptionType.hpp
        SymmetricEncryptor.cpp
        SymmetricEncryptor.hpp
        SymmetricIv.hpp
        SymmetricKey.hpp
        SymmetricNonce.hpp
        SymmetricTag.hpp
)
