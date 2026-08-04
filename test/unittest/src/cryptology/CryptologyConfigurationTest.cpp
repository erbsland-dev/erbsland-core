// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/cryptology/HashSelector.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/GaloisMultiplierFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/PortableAesBlockCipher.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/PortableGaloisMultiplier.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/ChaCha20BackendFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/Poly1305Factory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortableChaCha20Backend.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortablePoly1305.hpp>
#include <erbsland/cryptology/symmetric/SymmetricDecryptor.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptionSelector.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptor.hpp>
#include <erbsland/cryptology/tls/TlsConfiguration.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/text/Literals.hpp>

#if defined(ERBSLAND_CHACHA20_ARM_BACKEND)
#include <erbsland/cryptology/impl/algorithm/chacha20/ArmChaCha20Backend.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/ArmPoly1305.hpp>
#endif
#if defined(ERBSLAND_CHACHA20_X86_BACKEND)
#include <erbsland/cryptology/impl/algorithm/chacha20/X86ChaCha20Backend.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/X86Poly1305.hpp>
#endif

#include <array>
#include <atomic>
#include <thread>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(CryptologyConfiguration HashSelector SymmetricEncryptionSelector ApplicationDataImpl)
class CryptologyConfigurationTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testDefaultsDowngradesClearAndReset() {
        auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        REQUIRE(configuration.hardwareAccelerationEnabled());
        REQUIRE_FALSE(configuration.maximumStatus(HashAlgorithm::Sha3_256).has_value());
        REQUIRE_FALSE(configuration.maximumStatus(SymmetricEncryptionType::Aes256Gcm).has_value());

        configuration.setHardwareAccelerationEnabled(false);
        configuration.setMaximumStatus(HashAlgorithm::Sha3_256, CryptographicStatus::Legacy);
        configuration.setMaximumStatus(SymmetricEncryptionType::Aes256Gcm, CryptographicStatus::Disallowed);
        REQUIRE_FALSE(configuration.hardwareAccelerationEnabled());
        REQUIRE_EQUAL(HashSelector{}.status(HashAlgorithm::Sha3_256), CryptographicStatus::Legacy);
        REQUIRE_EQUAL(
            SymmetricEncryptionSelector{}.status(SymmetricEncryptionType::Aes256Gcm), CryptographicStatus::Disallowed);
        REQUIRE_EQUAL(HashSelector{}.recommended(), HashAlgorithm::Sha2_256);
        REQUIRE_EQUAL(SymmetricEncryptionSelector{}.recommended(), SymmetricEncryptionType::ChaCha20Poly1305);

        // A configured ceiling cannot promote the library's intrinsic legacy status.
        configuration.setMaximumStatus(SymmetricEncryptionType::Aes256CbcRandomFill, CryptographicStatus::Acceptable);
        REQUIRE_EQUAL(
            SymmetricEncryptionSelector{}.status(SymmetricEncryptionType::Aes256CbcRandomFill),
            CryptographicStatus::Legacy);

        configuration.clearMaximumStatus(HashAlgorithm::Sha3_256);
        REQUIRE_EQUAL(HashSelector{}.status(HashAlgorithm::Sha3_256), CryptographicStatus::Acceptable);
        configuration.reset();
        REQUIRE(configuration.hardwareAccelerationEnabled());
        REQUIRE_FALSE(configuration.maximumStatus(SymmetricEncryptionType::Aes256Gcm).has_value());
    }

    void testInvalidValuesAreRejected() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        const auto invalidHash = HashAlgorithm{static_cast<HashAlgorithm::Value>(0xffU)};
        const auto invalidType = SymmetricEncryptionType{};
        const auto invalidStatus = static_cast<CryptographicStatus>(0xffU);
        REQUIRE_THROWS_AS(
            el::err::ParameterError, configuration.setMaximumStatus(invalidHash, CryptographicStatus::Disallowed));
        REQUIRE_THROWS_AS(
            el::err::ParameterError, configuration.setMaximumStatus(invalidType, CryptographicStatus::Disallowed));
        REQUIRE_THROWS_AS(
            el::err::ParameterError, configuration.setMaximumStatus(HashAlgorithm::Sha3_256, invalidStatus));
        REQUIRE_EQUAL(HashSelector{}.status(invalidHash), CryptographicStatus::Disallowed);
        REQUIRE_FALSE(
            HashSelector{HashRequirements{.requiredStatus = CryptographicStatus::Disallowed}}.matches(invalidHash));
        REQUIRE_EQUAL(SymmetricEncryptionSelector{}.status(invalidType), CryptographicStatus::Disallowed);
    }

    void testTlsConfigurationExactParentAndGlobalResolution() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setTlsConfiguration(""_el, TlsConfiguration{});
        configuration.setTlsConfiguration("http"_el, TlsConfiguration{});
        configuration.setTlsConfiguration("http/client"_el, TlsConfiguration{});

        const auto exact = configuration.resolveTlsConfiguration("http/client"_el);
        REQUIRE_EQUAL(exact.requestedLabel(), "http/client"_el);
        REQUIRE_EQUAL(exact.matchedLabel(), "http/client"_el);
        REQUIRE(exact.configuration() != nullptr);
        const auto parent = configuration.resolveTlsConfiguration("http/client/internal"_el);
        REQUIRE_EQUAL(parent.requestedLabel(), "http/client/internal"_el);
        REQUIRE_EQUAL(parent.matchedLabel(), "http/client"_el);
        REQUIRE_EQUAL(parent.configuration(), exact.configuration());
        const auto frameworkParent = configuration.resolveTlsConfiguration("http/server"_el);
        REQUIRE_EQUAL(frameworkParent.matchedLabel(), "http"_el);
        const auto global = configuration.resolveTlsConfiguration("tls/client"_el);
        REQUIRE_EQUAL(global.matchedLabel(), ""_el);
        REQUIRE(configuration.hasTlsConfiguration("http/client"_el));
        REQUIRE_FALSE(configuration.hasTlsConfiguration("http/client/internal"_el));
    }

    void testTlsConfigurationResolutionSelectsCompleteEntry() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setTlsConfiguration(
            "tls"_el, TlsConfiguration{X509ServerCertificatePolicy{X509CertificateBundle{}}});
        configuration.setTlsConfiguration("tls/client"_el, TlsConfiguration{});

        const auto exact = configuration.resolveTlsConfiguration("tls/client/internal"_el);
        REQUIRE_EQUAL(exact.matchedLabel(), "tls/client"_el);
        REQUIRE_FALSE(exact.configuration()->hasServerCertificatePolicy());
        configuration.clearTlsConfiguration("tls/client"_el);
        const auto parent = configuration.resolveTlsConfiguration("tls/client/internal"_el);
        REQUIRE_EQUAL(parent.matchedLabel(), "tls"_el);
        REQUIRE(parent.configuration()->hasServerCertificatePolicy());
    }

    void testTlsConfigurationLabelsAreValidated() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        const auto invalidLabels = std::array{
            "/tls"_el,
            "tls/"_el,
            "tls//client"_el,
            "Tls/client"_el,
            "tls_client"_el,
            "-tls"_el,
            "tls-/client"_el,
            "a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a"_el};
        for (const auto &label : invalidLabels) {
            REQUIRE_THROWS_AS(el::err::ParameterError, configuration.setTlsConfiguration(label, TlsConfiguration{}));
            REQUIRE_THROWS_AS(el::err::ParameterError, configuration.hasTlsConfiguration(label));
            REQUIRE_THROWS_AS(el::err::ParameterError, configuration.resolveTlsConfiguration(label));
            REQUIRE_THROWS_AS(el::err::ParameterError, configuration.clearTlsConfiguration(label));
        }
        const auto tooLong = el::text::String::fromCharacter(U'a', el::unit::CpLength{256U});
        REQUIRE_THROWS_AS(el::err::ParameterError, configuration.setTlsConfiguration(tooLong, TlsConfiguration{}));
        configuration.setTlsConfiguration("a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a"_el, TlsConfiguration{});
        REQUIRE(configuration.hasTlsConfiguration("a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a"_el));
    }

    void testTlsConfigurationCapacityReplacementClearingAndReset() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        for (auto index = std::size_t{}; index < CryptologyConfiguration::cMaximumTlsConfigurations; ++index) {
            configuration.setTlsConfiguration(
                el::text::String::fromJoined({"entry-"_el, el::text::String::fromInteger(index)}), TlsConfiguration{});
        }
        configuration.setTlsConfiguration("entry-0"_el, TlsConfiguration{});
        REQUIRE_THROWS_AS(
            el::err::RuntimeError, configuration.setTlsConfiguration("one-too-many"_el, TlsConfiguration{}));
        configuration.clearTlsConfiguration("entry-1"_el);
        configuration.setTlsConfiguration("replacement"_el, TlsConfiguration{});
        REQUIRE(configuration.hasTlsConfiguration("replacement"_el));
        configuration.clearTlsConfigurations();
        REQUIRE_FALSE(configuration.hasTlsConfiguration("replacement"_el));
        configuration.setTlsConfiguration("tls"_el, TlsConfiguration{});
        configuration.reset();
        REQUIRE_FALSE(configuration.hasTlsConfiguration("tls"_el));
    }

    void testTlsConfigurationUnresolvedDiagnosticAndImmutableSnapshot() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        try {
            static_cast<void>(configuration.resolveTlsConfiguration("http/client/internal"_el));
            REQUIRE(false);
        } catch (const el::err::RuntimeError &error) {
            const auto diagnostic = error.toString();
            REQUIRE(diagnostic.contains("http/client/internal -> http/client -> http -> \"\""_el));
        }

        configuration.setTlsConfiguration("tls/client"_el, TlsConfiguration{});
        const auto first = configuration.resolveTlsConfiguration("tls/client"_el);
        configuration.setTlsConfiguration(
            "tls/client"_el, TlsConfiguration{X509ServerCertificatePolicy{X509CertificateBundle{}}});
        const auto second = configuration.resolveTlsConfiguration("tls/client"_el);
        REQUIRE(first.configuration() != second.configuration());
        REQUIRE_FALSE(first.configuration()->hasServerCertificatePolicy());
        REQUIRE(second.configuration()->hasServerCertificatePolicy());
    }

    void testTlsConfigurationConcurrentResolutionIsCoherent() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setTlsConfiguration("tls"_el, TlsConfiguration{});
        auto stop = std::atomic<bool>{false};
        auto writer = std::thread{[&]() -> void {
            while (!stop.load(std::memory_order_relaxed)) {
                configuration.setTlsConfiguration(
                    "tls/client"_el, TlsConfiguration{X509ServerCertificatePolicy{X509CertificateBundle{}}});
                configuration.clearTlsConfiguration("tls/client"_el);
            }
        }};
        for (auto iteration = std::size_t{}; iteration < 1000U; ++iteration) {
            const auto resolution = configuration.resolveTlsConfiguration("tls/client/internal"_el);
            REQUIRE(resolution.configuration() != nullptr);
            if (resolution.matchedLabel() == "tls/client"_el) {
                REQUIRE(resolution.configuration()->hasServerCertificatePolicy());
            } else {
                REQUIRE_EQUAL(resolution.matchedLabel(), "tls"_el);
                REQUIRE_FALSE(resolution.configuration()->hasServerCertificatePolicy());
            }
        }
        stop.store(true, std::memory_order_relaxed);
        writer.join();
    }

    void testApplicationDataSharesLazyConfiguration() {
        auto applicationScope = ApplicationTestScope<>{};
        auto &first = applicationScope.app().cryptologyConfiguration();
        auto &second = applicationScope.app().cryptologyConfiguration();
        REQUIRE_EQUAL(&first, &second);
        first.setHardwareAccelerationEnabled(false);

        // Application instances registered or linked in another module share the same ApplicationData allocation.
        auto linkedApplication = el::core::Application{};
        el::core::Application::linkWith(applicationScope.app());
        REQUIRE_EQUAL(&linkedApplication.cryptologyConfiguration(), &first);
        REQUIRE_FALSE(linkedApplication.cryptologyConfiguration().hardwareAccelerationEnabled());
    }

    void testDisabledAccelerationForcesEveryPortableFactory() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setHardwareAccelerationEnabled(false);
        const auto aesKey = bytesFromHex("000102030405060708090a0b0c0d0e0f");
        const auto chachaKey = bytesFromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
        const auto nonce = bytesFromHex("000000090000004a00000000");

        const auto aes = el::cryptology::impl::createAesBlockCipher(aesKey.span());
        const auto gHash = el::cryptology::impl::createGaloisMultiplier();
        const auto chacha = el::cryptology::impl::createChaCha20Backend(chachaKey.span(), nonce.span());
        const auto poly1305 = el::cryptology::impl::createPoly1305(chachaKey.span());
        REQUIRE(dynamic_cast<el::cryptology::impl::PortableAesBlockCipher *>(aes.get()) != nullptr);
        REQUIRE(dynamic_cast<el::cryptology::impl::PortableGaloisMultiplier *>(gHash.get()) != nullptr);
        REQUIRE(dynamic_cast<el::cryptology::impl::PortableChaCha20Backend *>(chacha.get()) != nullptr);
        REQUIRE(dynamic_cast<el::cryptology::impl::PortablePoly1305 *>(poly1305.get()) != nullptr);
#if defined(ERBSLAND_CHACHA20_ARM_BACKEND)
        REQUIRE(dynamic_cast<el::cryptology::impl::ArmChaCha20Backend *>(chacha.get()) == nullptr);
        REQUIRE(dynamic_cast<el::cryptology::impl::ArmPoly1305 *>(poly1305.get()) == nullptr);
#endif
#if defined(ERBSLAND_CHACHA20_X86_BACKEND)
        REQUIRE(dynamic_cast<el::cryptology::impl::X86ChaCha20Backend *>(chacha.get()) == nullptr);
        REQUIRE(dynamic_cast<el::cryptology::impl::X86Poly1305 *>(poly1305.get()) == nullptr);
#endif
    }

    void testPolicyDoesNotDisableExplicitUse() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        for (const auto algorithm : HashAlgorithm::all()) {
            configuration.setMaximumStatus(algorithm, CryptographicStatus::Disallowed);
        }
        for (const auto type : SymmetricEncryptionType::all()) {
            configuration.setMaximumStatus(type, CryptographicStatus::Disallowed);
        }
        REQUIRE(HashSelector{}.allAccepted().isEmpty());
        REQUIRE_FALSE(HashSelector{}.recommended().has_value());
        REQUIRE(SymmetricEncryptionSelector{}.allAccepted().isEmpty());
        REQUIRE_FALSE(SymmetricEncryptionSelector{}.recommended().has_value());

        auto hasher = Hasher{HashAlgorithm::Sha3_256};
        hasher.update(bytesFromHex("00"));
        REQUIRE_EQUAL(hasher.finalize().length(), el::unit::ByteLength{32U});

        configuration.setMaximumStatus(SymmetricEncryptionType::ChaCha20Poly1305, CryptographicStatus::Disallowed);
        const auto key = SymmetricKey{bytesFromHex("0000000000000000000000000000000000000000000000000000000000000000")};
        const auto nonce = SymmetricNonce{bytesFromHex("000000000000000000000000")};
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::ChaCha20Poly1305, key, nonce};
        const auto ciphertext = encryptor.encrypt(bytesFromHex("00"));
        REQUIRE(encryptor.finalize().isEmpty());
        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::ChaCha20Poly1305, key, nonce};
        REQUIRE_EQUAL(decryptor.decrypt(ciphertext), bytesFromHex("00"));
        REQUIRE(decryptor.finalize(encryptor.tag()).isEmpty());
    }

    void testSelectorListsUseCoherentSnapshotsDuringLiveChanges() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        auto stop = std::atomic<bool>{false};
        auto writer = std::thread{[&]() -> void {
            while (!stop.load(std::memory_order_relaxed)) {
                configuration.setMaximumStatus(SymmetricEncryptionType::Aes256Gcm, CryptographicStatus::Disallowed);
                configuration.clearMaximumStatus(SymmetricEncryptionType::Aes256Gcm);
            }
        }};
        for (auto iteration = std::size_t{}; iteration < 1000U; ++iteration) {
            const auto accepted = SymmetricEncryptionSelector{}.allAccepted().toStdVector();
            REQUIRE(accepted.size() == 2U || accepted.size() == 3U);
            if (accepted.size() == 2U) {
                REQUIRE_EQUAL(accepted[0], SymmetricEncryptionType::ChaCha20Poly1305);
                REQUIRE_EQUAL(accepted[1], SymmetricEncryptionType::Aes128Gcm);
            } else {
                REQUIRE_EQUAL(accepted[0], SymmetricEncryptionType::Aes256Gcm);
                REQUIRE_EQUAL(accepted[1], SymmetricEncryptionType::ChaCha20Poly1305);
                REQUIRE_EQUAL(accepted[2], SymmetricEncryptionType::Aes128Gcm);
            }
        }
        stop.store(true, std::memory_order_relaxed);
        writer.join();
    }
};
