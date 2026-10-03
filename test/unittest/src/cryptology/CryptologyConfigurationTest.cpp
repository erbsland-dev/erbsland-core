// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/GaloisMultiplierFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/PortableAesBlockCipher.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/PortableGaloisMultiplier.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/ChaCha20BackendFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/Poly1305Factory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortableChaCha20Backend.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortablePoly1305.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/CryptologyError.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/HashSelector.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/SigningPrivateKey.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/SymmetricDecryptor.hpp>
#include <erbsland/SymmetricEncryptionSelector.hpp>
#include <erbsland/SymmetricEncryptor.hpp>
#include <erbsland/TlsConfiguration.hpp>

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
            el::err::ParameterFailure, configuration.setMaximumStatus(invalidHash, CryptographicStatus::Disallowed));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, configuration.setMaximumStatus(invalidType, CryptographicStatus::Disallowed));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, configuration.setMaximumStatus(HashAlgorithm::Sha3_256, invalidStatus));
        REQUIRE_EQUAL(HashSelector{}.status(invalidHash), CryptographicStatus::Disallowed);
        REQUIRE_FALSE(
            HashSelector{HashRequirements{.requiredStatus = CryptographicStatus::Disallowed}}.matches(invalidHash));
        REQUIRE_EQUAL(SymmetricEncryptionSelector{}.status(invalidType), CryptographicStatus::Disallowed);
    }

    void testTlsDefaultsAndReservedLabels() {
        auto scope = ApplicationTestScope<>{};
        auto &registry = scope.app().cryptologyConfiguration();
        for (const auto label : {""_el, "http"_el, "custom/misspelled"_el}) {
            const auto resolution = registry.resolveTlsConfiguration(label, TlsRole::Client);
            REQUIRE_EQUAL(resolution.requestedLabel(), label);
            REQUIRE(resolution.matchedLabel().isEmpty());
            REQUIRE_EQUAL(resolution.role(), TlsRole::Client);
            REQUIRE(resolution.certificatePolicy()->isSystemTrust());
            REQUIRE_FALSE(resolution.configuration()->client()->identity());
        }
        REQUIRE(registry.hasTlsConfiguration(""_el, TlsRole::Client));
        REQUIRE(registry.hasTlsConfiguration(""_el, TlsRole::Server));
        REQUIRE_THROWS_AS(el::CryptologyError, registry.resolveTlsConfiguration("http"_el, TlsRole::Server));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, registry.setTlsConfiguration(""_el, TlsConfiguration{TlsClientConfiguration{}}));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, registry.clearTlsConfiguration(""_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, registry.clearTlsConfiguration(""_el, TlsRole::Client));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, registry.setTlsConfiguration("empty"_el, TlsConfiguration{}));
    }

    void testTlsRolesParentsReplacementAndSnapshots() {
        auto scope = ApplicationTestScope<>{};
        auto &registry = scope.app().cryptologyConfiguration();
        registry.setTlsConfiguration("http"_el, serverProfile());
        registry.setTlsConfiguration("http"_el, TlsConfiguration{TlsClientConfiguration{}});
        registry.setTlsConfiguration("http/internal"_el, serverProfile());
        const auto client = registry.resolveTlsConfiguration("http/internal"_el, TlsRole::Client);
        REQUIRE_EQUAL(client.matchedLabel(), "http"_el);
        const auto server = registry.resolveTlsConfiguration("http/internal"_el, TlsRole::Server);
        REQUIRE_EQUAL(server.matchedLabel(), "http/internal"_el);
        REQUIRE_FALSE(server.certificatePolicy());
        REQUIRE(registry.hasTlsConfiguration("http"_el, TlsRole::Server));
        REQUIRE_FALSE(registry.hasTlsConfiguration("http/internal"_el, TlsRole::Client));
        registry.clearTlsConfiguration("http"_el, TlsRole::Client);
        REQUIRE(registry.hasTlsConfiguration("http"_el, TlsRole::Server));
        REQUIRE(registry.resolveTlsConfiguration("http/internal"_el, TlsRole::Client).matchedLabel().isEmpty());
        REQUIRE(client.configuration()->client().has_value());
        registry.clearTlsConfigurations();
        REQUIRE(server.configuration()->server()->identity());
        REQUIRE_FALSE(registry.hasTlsConfiguration("http"_el, TlsRole::Server));
    }

    void testTlsTrustPrecedence() {
        const auto anchors = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/ca.pem")});
        for (const auto global : {std::optional<bool>{}, std::optional<bool>{false}, std::optional<bool>{true}}) {
            auto settings = TlsSettings{};
            settings.setSystemTrust(global);
            for (const auto role : {TlsRole::Client, TlsRole::Server}) {
                auto trust = TlsTrustConfiguration{};
                if (global.value_or(role == TlsRole::Client)) {
                    REQUIRE(trust.resolve(settings, role)->isSystemTrust());
                } else {
                    REQUIRE_THROWS_AS(el::CryptologyError, trust.resolve(settings, role));
                }
                trust.setTrustAnchors(anchors);
                REQUIRE_FALSE(trust.resolve(settings, role)->isSystemTrust());
                trust.setSystemTrust(true);
                REQUIRE_THROWS_AS(el::CryptologyError, trust.resolve(settings, role));
                trust.setSystemTrust(false);
                REQUIRE_FALSE(trust.resolve(settings, role)->isSystemTrust());
            }
        }
        auto trust = TlsTrustConfiguration{};
        trust.setSystemTrust(false);
        REQUIRE_THROWS_AS(el::CryptologyError, trust.resolve(TlsSettings{}, TlsRole::Client));
        REQUIRE_FALSE(trust.resolve(TlsSettings{}, TlsRole::Server, false));
        trust.setIntermediates(anchors);
        REQUIRE_THROWS_AS(el::CryptologyError, trust.resolve(TlsSettings{}, TlsRole::Server, false));
    }

    void testTlsAtomicReplacementAndReset() {
        auto scope = ApplicationTestScope<>{};
        auto &registry = scope.app().cryptologyConfiguration();
        registry.setTlsConfiguration("original"_el, TlsConfiguration{TlsClientConfiguration{}});
        const auto original = registry.resolveTlsConfiguration("original"_el, TlsRole::Client);
        auto settings = TlsSettings{};
        settings.setSystemTrust(false);
        auto replacement = TlsConfigurationSet{};
        replacement.setSettings(settings);
        replacement.add("invalid"_el, TlsConfiguration{TlsClientConfiguration{}});
        REQUIRE_THROWS_AS(el::CryptologyError, registry.replaceTlsConfigurations(replacement));
        REQUIRE(registry.hasTlsConfiguration("original"_el, TlsRole::Client));
        REQUIRE_FALSE(registry.tlsSettings().systemTrust());
        replacement = TlsConfigurationSet{};
        replacement.add("duplicate"_el, TlsConfiguration{TlsClientConfiguration{}});
        replacement.add("duplicate"_el, TlsConfiguration{TlsClientConfiguration{}});
        REQUIRE_THROWS_AS(el::CryptologyError, registry.replaceTlsConfigurations(replacement));
        replacement = TlsConfigurationSet{};
        replacement.setSettings(settings);
        registry.replaceTlsConfigurations(replacement);
        REQUIRE_FALSE(registry.hasTlsConfiguration("original"_el, TlsRole::Client));
        REQUIRE_THROWS_AS(el::CryptologyError, registry.resolveTlsConfiguration("unknown"_el, TlsRole::Client));
        registry.clearTlsConfigurations();
        REQUIRE_EQUAL(registry.tlsSettings().systemTrust(), std::optional<bool>{false});
        REQUIRE(original.certificatePolicy()->isSystemTrust());
        registry.reset();
        REQUIRE(registry.resolveTlsConfiguration("unknown"_el, TlsRole::Client).certificatePolicy()->isSystemTrust());
    }

    void testTlsLabelsAndCapacity() {
        auto scope = ApplicationTestScope<>{};
        auto &registry = scope.app().cryptologyConfiguration();
        for (
            const auto label :
            {"/tls"_el,
                "tls/"_el,
                "tls//client"_el,
                "Tls"_el,
                "tls_client"_el,
                "-tls"_el,
                "tls-"_el,
                "a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a/a"_el}) {
            REQUIRE_THROWS_AS(
                el::err::ParameterFailure,
                registry.setTlsConfiguration(label, TlsConfiguration{TlsClientConfiguration{}}));
            REQUIRE_THROWS_AS(el::err::ParameterFailure, registry.resolveTlsConfiguration(label, TlsRole::Client));
            REQUIRE_THROWS_AS(el::err::ParameterFailure, registry.hasTlsConfiguration(label, TlsRole::Server));
            REQUIRE_THROWS_AS(el::err::ParameterFailure, registry.clearTlsConfiguration(label));
        }
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, registry.resolveTlsConfiguration("tls"_el, static_cast<TlsRole>(255)));
        for (auto i = std::size_t{}; i < CryptologyConfiguration::cMaximumTlsConfigurations; ++i) {
            registry.setTlsConfiguration(
                el::join("entry-"_el, el::text::String::fromInteger(i)), TlsConfiguration{TlsClientConfiguration{}});
        }
        registry.setTlsConfiguration("entry-0"_el, TlsConfiguration{TlsClientConfiguration{}});
        REQUIRE_THROWS_AS(
            el::CryptologyError,
            registry.setTlsConfiguration("overflow"_el, TlsConfiguration{TlsClientConfiguration{}}));
        registry.clearTlsConfiguration("entry-1"_el);
        registry.setTlsConfiguration("replacement"_el, TlsConfiguration{TlsClientConfiguration{}});
    }

    void testTlsConcurrentBatchResolutionIsCoherent() {
        auto scope = ApplicationTestScope<>{};
        auto &registry = scope.app().cryptologyConfiguration();
        auto first = TlsConfigurationSet{};
        first.add("http"_el, TlsConfiguration{TlsClientConfiguration{}});
        auto second = TlsConfigurationSet{};
        second.add("http/internal"_el, TlsConfiguration{TlsClientConfiguration{}});
        registry.replaceTlsConfigurations(first);
        auto writer = std::jthread{[&](std::stop_token stop) -> void {
            while (!stop.stop_requested()) {
                registry.replaceTlsConfigurations(first);
                registry.replaceTlsConfigurations(second);
            }
        }};
        for (auto i = 0; i < 100; ++i) {
            const auto values = registry.resolveTlsConfigurations(
                el::StringList{"http/internal/a"_el, "http/internal/b"_el}, TlsRole::Client);
            REQUIRE_EQUAL(values[0].matchedLabel(), values[1].matchedLabel());
        }
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
        REQUIRE_EQUAL(hasher.finalize().length(), el::unit::ByteCount{32U});

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

private:
    static auto serverProfile() -> TlsConfiguration {
        auto identity = TlsIdentity{
            X509CertificateBundle::fromPemOrThrow(
                el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/server-ecdsa.pem")}),
            SigningPrivateKey::fromPemOrThrow(
                el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/server-ecdsa-key.pem")})};
        return TlsConfiguration{TlsServerConfiguration{std::move(identity)}};
    }
};
