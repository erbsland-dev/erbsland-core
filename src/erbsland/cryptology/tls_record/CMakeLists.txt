# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        TlsRecordContentType.hpp
        TlsRecordDecryptor.cpp
        TlsRecordDecryptor.hpp
        TlsRecordEncryptor.cpp
        TlsRecordEncryptor.hpp
        TlsRecordError.cpp
        TlsRecordError.hpp
        TlsRecordErrorCategory.hpp
        TlsRecordPlaintext.cpp
        TlsRecordPlaintext.hpp
        TlsTrafficSecret.cpp
        TlsTrafficSecret.hpp
)
