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
        AuthenticationTokenTest.cpp
        ChaCha20BackendFullTest.cpp
        ChaCha20Poly1305FullTest.cpp
        ChaCha20Poly1305Test.cpp
        ChaCha20PrimitiveTest.cpp
        CryptographicDataBlockTest.cpp
        CryptologyConfigurationTest.cpp
        CryptologyResponseKeys.hpp
        CryptologyResponseReader.cpp
        CryptologyResponseReader.hpp
        CryptologyTestHelper.hpp
        CryptologyVectorManifestTest.cpp
        DeterministicSecureRandom.hpp
        EcdsaSignatureFullTest.cpp
        EcdsaSignatureTest.cpp
        EcPrivateScalarPolicyTest.cpp
        EcSigningKeyTest.cpp
        EcSigningKeyTestApplication.hpp
        EcSigningKeyTestHelper.hpp
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
        MockNativeTlsSigner.hpp
        MockSecureStorageBackend.hpp
        MockSystemIdentityBackend.hpp
        ModernSignatureTest.cpp
        NistPrimeNumberTest.cpp
        P256KeyAgreementTest.cpp
        PasswordHasherTest.cpp
        PasswordPrimitiveTest.cpp
        ProtectedByteBlockTest.cpp
        RsaSignatureFullTest.cpp
        RsaSignatureTest.cpp
        ScriptedSecureRandom.hpp
        SecureStorageSourceTest.cpp
        SecureStorageTest.cpp
        SecureStorageTestAccess.hpp
        Sha3ValidationTest.cpp
        SignedByteBlockTest.cpp
        SigningPrivateKeyTest.cpp
        SymmetricEncryptionFrontendTest.cpp
        SymmetricEncryptionTypeTest.cpp
        SystemIdentityStoreTest.cpp
        SystemIdentityStoreTestAccess.hpp
        SystemTrustTest.cpp
        Tls13KeyScheduleTest.cpp
        TlsConfigurationParserTest.cpp
        TlsIdentityTest.cpp
        TlsRecordProtectionTest.cpp
        TlsSignatureSchemeFullTest.cpp
        TlsSignatureSchemeTest.cpp
        X25519FullTest.cpp
        X509CertificateBackendTest.cpp
        X509CertificateBuilderTest.cpp
        X509CertificateFileTest.cpp
        X509CertificateTest.cpp
        X509TestData.hpp
        X509TlsCertificatePolicyTest.cpp
)

if (APPLE OR WIN32)
    target_sources(erbsland-core-unittest PRIVATE
            SecureStorageNativeTest.cpp
    )
endif ()

if (APPLE)
    target_sources(erbsland-core-unittest PRIVATE
            MacosIdentityTestFixture.hpp
            MacosSystemIdentityNativeTest.cpp
    )
elseif (WIN32)
    target_sources(erbsland-core-unittest PRIVATE
            WindowsSystemIdentityNativeTest.cpp
    )
endif ()
