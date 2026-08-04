# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        TlsCipherSuite.cpp
        TlsCipherSuite.hpp
        TlsConfiguration.hpp
        TlsConfiguration_fwd.hpp
        TlsConfigurationResolution.hpp
        TlsServerIdentity.cpp
        TlsServerIdentity.hpp
        TlsServerIdentity_fwd.hpp
        TlsSignatureScheme.cpp
        TlsSignatureScheme.hpp
        TlsSignatureScheme_fwd.hpp
)
