# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        SystemIdentityInfo.hpp
        SystemIdentityStore.cpp
        SystemIdentityStore.hpp
        SystemIdentityStore_fwd.hpp
        TlsCipherSuite.cpp
        TlsCipherSuite.hpp
        TlsClientAuthentication.hpp
        TlsClientConfiguration.hpp
        TlsConfiguration.hpp
        TlsConfiguration_fwd.hpp
        TlsConfigurationEntry.hpp
        TlsConfigurationParser.cpp
        TlsConfigurationParser.hpp
        TlsConfigurationParser_fwd.hpp
        TlsConfigurationResolution.hpp
        TlsConfigurationSet.hpp
        TlsIdentity.cpp
        TlsIdentity.hpp
        TlsIdentity_fwd.hpp
        TlsRole.hpp
        TlsServerConfiguration.hpp
        TlsSettings.hpp
        TlsSignatureScheme.cpp
        TlsSignatureScheme.hpp
        TlsSignatureScheme_fwd.hpp
        TlsTrustConfiguration.hpp
)
