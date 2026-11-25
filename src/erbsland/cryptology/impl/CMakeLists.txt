# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(algorithm)
add_subdirectory(authentication)
add_subdirectory(hash)
add_subdirectory(key_agreement)
add_subdirectory(protected_data)
add_subdirectory(secure_storage)
add_subdirectory(symmetric)
add_subdirectory(system_trust)
add_subdirectory(tls)

target_sources(erbsland-core PRIVATE
        CryptologyAsn1Limits.hpp
        AuthenticationChallengeTestAccess_fwd.hpp
        AuthenticationCodec.cpp
        AuthenticationCodec.hpp
        CryptologyConfigurationSnapshot.cpp
        CryptologyConfigurationSnapshot.hpp
        CryptologyOids.hpp
        EcPrivateScalarPolicy.cpp
        EcPrivateScalarPolicy.hpp
        EncryptedPrivateKeyCodec.cpp
        EncryptedPrivateKeyCodec.hpp
        PasswordHashData.cpp
        PasswordHashData.hpp
        PasswordHashData_fwd.hpp
        PemCodec.cpp
        PemCodec.hpp
        PemDerFileTools.cpp
        PemDerFileTools.hpp
        PemLabel.hpp
        PortableX509CertificateData.hpp
        PortableX509CertificateData_fwd.hpp
        PortableX509CertificateValues.hpp
        PrivateKeyParser.cpp
        PrivateKeyParser.hpp
        PrivateKeyParser_fwd.hpp
        NativeTlsAlgorithms.cpp
        NativeTlsAlgorithms.hpp
        SecureEraseGuard.hpp
        SigningKeyEncoding.cpp
        SigningKeyEncoding.hpp
        SigningKeyGenerator.cpp
        SigningKeyGenerator.hpp
        SigningKeyGenerator_fwd.hpp
        SoftwareTlsSigner.hpp
        SystemIdentityBackend.cpp
        SystemIdentityBackend.hpp
        SystemIdentityBackend_fwd.hpp
        SystemIdentityMaterial.hpp
        TlsSigner.hpp
        TlsSigner_fwd.hpp
        UnsupportedSystemIdentityBackend.hpp
        TlsConfigurationLabel.cpp
        TlsConfigurationLabel.hpp
        TlsConfigurationRules.cpp
        TlsConfigurationRules.hpp
        X509ArtifactWriter.cpp
        X509ArtifactWriter.hpp
        X509ArtifactWriter_fwd.hpp
        X509CertificateBackendAccess.hpp
        X509CertificateBackendAccess_fwd.hpp
        X509CertificateData.hpp
        X509Parser.cpp
        X509Parser.hpp
        X509Parser_extensions.cpp
        X509Parser_fwd.hpp
        X509TlsCertificateValidator.cpp
        X509TlsCertificateValidator.hpp
        X509TlsCertificateValidator_fwd.hpp
        X509TlsCertificateValidator_native.cpp
        X509TlsCertificateValidator_validation.cpp
)

if (APPLE)
    target_sources(erbsland-core PRIVATE
        CryptologyAsn1Limits.hpp
            MacosKeychainInteraction.cpp
            MacosKeychainInteraction.hpp
            MacosOwnedReference.hpp
            MacosSystemIdentityBackend.cpp
            MacosSystemIdentityBackend.hpp
            MacosTlsSigner.cpp
            MacosTlsSigner.hpp
    )
elseif (WIN32)
    target_sources(erbsland-core PRIVATE
        CryptologyAsn1Limits.hpp
            WindowsIdentityResources.hpp
            WindowsSystemIdentityBackend.cpp
            WindowsSystemIdentityBackend.hpp
            WindowsTlsSigner.cpp
            WindowsTlsSigner.hpp
    )
    target_link_libraries(erbsland-core PRIVATE ncrypt crypt32)
endif ()
