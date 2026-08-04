# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        AesCbcFullTest.cpp
        AesCbcTest.cpp
        AesCbcTestApplication.hpp
        AesGcmFullTest.cpp
        AesGcmTest.cpp
        AesPrimitiveFullTest.cpp
        AesPrimitiveTest.cpp
        Asn1NodeTest.cpp
        ChaCha20BackendFullTest.cpp
        ChaCha20Poly1305FullTest.cpp
        ChaCha20Poly1305Test.cpp
        ChaCha20PrimitiveTest.cpp
        CryptographicDataBlockTest.cpp
        CryptologyConfigurationTest.cpp
        CryptologyResponseReader.cpp
        CryptologyResponseReader.hpp
        CryptologyTestHelper.hpp
        CryptologyVectorManifestTest.cpp
        DeterministicSecureRandom.hpp
        EcdsaSignatureFullTest.cpp
        EcdsaSignatureTest.cpp
        Ed25519SignatureFullTest.cpp
        Ed25519SignatureTest.cpp
        HashAlgorithmTest.cpp
        HasherTest.cpp
        HashFullValidationTest.cpp
        HashPrimitiveFullValidationTest.cpp
        HashValidationTest.cpp
        HkdfTest.cpp
        HmacTest.cpp
        KeyAgreementTest.cpp
        PasswordHashTest.cpp
        PasswordPrimitiveTest.cpp
        ProtectedByteBlockTest.cpp
        RsaSignatureFullTest.cpp
        RsaSignatureTest.cpp
        Sha3ValidationTest.cpp
        SigningPrivateKeyTest.cpp
        SymmetricEncryptionFrontendTest.cpp
        SymmetricEncryptionTypeTest.cpp
        Tls13KeyScheduleTest.cpp
        TlsRecordProtectionTest.cpp
        TlsServerIdentityTest.cpp
        TlsSignatureSchemeFullTest.cpp
        TlsSignatureSchemeTest.cpp
        X25519FullTest.cpp
        X509CertificateBackendTest.cpp
        X509CertificateFileTest.cpp
        X509CertificateTest.cpp
        X509ServerCertificatePolicyTest.cpp
        X509TestData.hpp
)
