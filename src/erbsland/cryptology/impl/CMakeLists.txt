# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(algorithm)
add_subdirectory(authentication)
add_subdirectory(hash)
add_subdirectory(key_agreement)
add_subdirectory(protected_data)
add_subdirectory(symmetric)
add_subdirectory(tls)

target_sources(erbsland-core PRIVATE
        Asn1ObjectIdentifierCodec.cpp
        Asn1ObjectIdentifierCodec.hpp
        CryptologyConfigurationSnapshot.cpp
        CryptologyConfigurationSnapshot.hpp
        CryptologyOids.hpp
        DerEncoder.cpp
        DerEncoder.hpp
        DerParser.cpp
        DerParser.hpp
        DerParser_fwd.hpp
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
        SecureEraseGuard.hpp
        SigningKeyEncoding.cpp
        SigningKeyEncoding.hpp
        SigningKeyGenerator.cpp
        SigningKeyGenerator.hpp
        SigningKeyGenerator_fwd.hpp
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
        X509ServerCertificateValidator.cpp
        X509ServerCertificateValidator.hpp
        X509ServerCertificateValidator_fwd.hpp
        X509ServerCertificateValidator_validation.cpp
)
