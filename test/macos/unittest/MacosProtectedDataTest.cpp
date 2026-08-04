// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/impl/protected_data/ProtectedDataProvider.hpp>
#include <erbsland/cryptology/protected_data/ProtectedByteBlock.hpp>
#include <erbsland/cryptology/protected_data/ProtectedDataMode.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <utility>

using namespace el::cryptology;

TESTED_TARGETS(ProtectedByteBlock CryptologyConfiguration MacosProtectedDataProvider)
class MacosProtectedDataTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto plaintext() -> el::ByteBlock {
        return el::ByteBlock{
            el::Byte{0x00U},
            el::Byte{0x01U},
            el::Byte{0x7fU},
            el::Byte{0x80U},
            el::Byte{0xfeU},
            el::Byte{0xffU},
            el::Byte{0x42U},
            el::Byte{0xa5U},
        };
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPlatformOnlyConfigurationAndRoundTrip() {
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setProtectedDataMode(ProtectedDataMode::PlatformOnly);

        // PlatformOnly turns any Secure Enclave initialization or self-test failure into a test failure; no internal
        // AES fallback can satisfy these assertions.
        REQUIRE_NOTHROW(configuration.validateProtectedDataSupport());

        const auto source = plaintext();
        const auto protectedData = ProtectedByteBlock{source};
        REQUIRE_EQUAL(protectedData.byteLength(), source.length());
        REQUIRE_EQUAL(protectedData.unprotect(), source);

        auto copied = protectedData;
        auto moved = std::move(copied);
        REQUIRE(copied.isEmpty());
        REQUIRE_EQUAL(moved.unprotect(), source);

        auto callbackCalled = false;
        moved.withUnprotectedData([&](const el::ConstByteSpan data) -> void {
            callbackCalled = true;
            REQUIRE_EQUAL(el::ByteBlock::fromSpan(data), source);
        });
        REQUIRE(callbackCalled);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNativeProviderAuthentication() {
        const auto source = plaintext();
        auto provider = el::cryptology::impl::ProtectedDataProvider::createPlatform();
        REQUIRE(provider != nullptr);

        const auto firstEnvelope = provider->protect(source.span(), source.length());
        const auto secondEnvelope = provider->protect(source.span(), source.length());
        REQUIRE_NOT_EQUAL(firstEnvelope, secondEnvelope);
        REQUIRE_EQUAL(provider->unprotect(firstEnvelope.span(), source.length()), source);
        REQUIRE_EQUAL(provider->unprotect(secondEnvelope.span(), source.length()), source);

        auto tamperedEnvelope = el::ByteBlockEditor{firstEnvelope};
        tamperedEnvelope.set(
            el::ByteIndex::zero(), tamperedEnvelope.getOrThrow(el::ByteIndex::zero()) ^ el::Byte{0x01U});
        REQUIRE_THROWS_AS(CryptologyError, provider->unprotect(tamperedEnvelope.span(), source.length()));

        const auto wrongLength = el::ByteLength::fromSizeT(source.length().toSizeT() + 1U);
        REQUIRE_THROWS_AS(CryptologyError, provider->unprotect(firstEnvelope.span(), wrongLength));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNativeKeyExpiresWithProvider() {
        const auto source = plaintext();
        auto envelope = el::ByteBlock{};
        {
            auto provider = el::cryptology::impl::ProtectedDataProvider::createPlatform();
            REQUIRE(provider != nullptr);
            envelope = provider->protect(source.span(), source.length());
            REQUIRE_EQUAL(provider->unprotect(envelope.span(), source.length()), source);
        }

        auto replacementProvider = el::cryptology::impl::ProtectedDataProvider::createPlatform();
        REQUIRE(replacementProvider != nullptr);
        REQUIRE_THROWS_AS(CryptologyError, replacementProvider->unprotect(envelope.span(), source.length()));
    }
};
