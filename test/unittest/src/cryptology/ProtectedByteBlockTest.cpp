// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/cryptology/impl/protected_data/InternalProtectedDataProvider.hpp>
#include <erbsland/cryptology/impl/protected_data/ProtectedDataAccess.hpp>
#include <erbsland/cryptology/impl/protected_data/ProtectedDataProvider.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/CryptologyError.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ProtectedByteBlock.hpp>
#include <erbsland/ProtectedDataMode.hpp>

#include <algorithm>
#include <atomic>
#include <limits>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

using namespace el::cryptology;

namespace erbsland::cryptology::impl {

class ProtectedDataTestAccess final {
public:
    /// Bind access to the target instance, which must outlive this helper.
    /// @param block The accessed instance.
    explicit ProtectedDataTestAccess(ProtectedByteBlock &block) noexcept : _block{block} {}

public: // access
    [[nodiscard]] auto envelope() -> const mem::ByteBlock & { return _block._envelope; }

    void corruptEnvelope() {
        auto editor = mem::ByteBlockEditor{_block._envelope};
        editor.set(unit::ByteIndex::zero(), editor.getOrThrow(unit::ByteIndex::zero()) ^ mem::Byte{0x01U});
        _block._envelope = editor;
    }

private:
    ProtectedByteBlock &_block; ///< The accessed instance.
};

/// Access the nonce state of a provider under test.
class ProtectedDataProviderTestAccess final {
public:
    explicit ProtectedDataProviderTestAccess(InternalProtectedDataProvider &provider) noexcept : _provider{provider} {}

public: // test state
    void exhaustNonce() { _provider._nonceCounter = std::numeric_limits<uint64_t>::max(); }

private:
    InternalProtectedDataProvider &_provider;
};

}

class ThrowingProtectedDataAccess final : public el::cryptology::impl::ProtectedDataAccess {
public:
    [[nodiscard]] auto protect(el::ConstByteSpan, el::ByteCount) -> el::ByteBlock override {
        throw CryptologyError{"Unexpected protected-data access."};
    }

    [[nodiscard]] auto unprotect(el::ConstByteSpan, el::ByteCount) -> el::ByteBlock override {
        throw CryptologyError{"Unexpected protected-data access."};
    }
};

class ProtectedDataAccessReset final {
public:
    ~ProtectedDataAccessReset() { el::cryptology::impl::ProtectedDataAccess::setTestAccess(nullptr); }
};

class ThrowingProtectedDataProvider final : public el::cryptology::impl::ProtectedDataProvider {
public:
    [[nodiscard]] static auto create() -> el::cryptology::impl::ProtectedDataProviderPtr {
        ++creationCount;
        return std::make_unique<ThrowingProtectedDataProvider>();
    }

    [[nodiscard]] auto protect(el::ConstByteSpan, el::ByteCount) -> el::ByteBlock override {
        throw CryptologyError{"Forced provider self-test failure."};
    }

    [[nodiscard]] auto unprotect(el::ConstByteSpan, el::ByteCount) -> el::ByteBlock override {
        throw CryptologyError{"Forced provider self-test failure."};
    }

    inline static std::atomic<std::size_t> creationCount{0U};
};

class ProtectedDataFactoryReset final {
public:
    ~ProtectedDataFactoryReset() { el::cryptology::impl::ProtectedDataProvider::setTestPlatformFactory(nullptr); }
};

TESTED_TARGETS(ProtectedByteBlock CryptologyConfiguration)
class ProtectedByteBlockTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    static_assert(std::is_copy_constructible_v<ProtectedByteBlock>);
    static_assert(std::is_copy_assignable_v<ProtectedByteBlock>);

    class EraseObserverGuard final {
    public:
        EraseObserverGuard() {
            _fourByteEraseCount = 0U;
            _keyEraseCount = 0U;
            el::mem::impl::setSecureEraseObserver(observeErase);
        }
        ~EraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
    };

    static void observeErase(const std::span<const std::byte> bytes) noexcept {
        if (bytes.size() == 4U &&
            std::ranges::all_of(bytes, [](const std::byte value) noexcept { return value == std::byte{}; })) {
            ++_fourByteEraseCount;
        }
        if (bytes.size() == 32U &&
            std::ranges::all_of(bytes, [](const std::byte value) noexcept { return value == std::byte{}; })) {
            ++_keyEraseCount;
        }
    }

    inline static std::atomic<std::size_t> _fourByteEraseCount{0U};
    inline static std::atomic<std::size_t> _keyEraseCount{0U};

public:
    void testEmptyWithoutApplication() {
        const auto applicationScope = ApplicationTestScope<>{ApplicationTestScope<>::NoLocalAppInstance{}};
        auto first = ProtectedByteBlock{};
        auto second = first;
        auto third = std::move(second);
        REQUIRE(first.isEmpty());
        REQUIRE(third.isEmpty());
        REQUIRE(first.unprotect().isEmpty());
        auto callbackCalled = false;
        first.withUnprotectedData([&](const el::ConstByteSpan data) -> void {
            callbackCalled = true;
            REQUIRE(data.empty());
        });
        REQUIRE(callbackCalled);
        REQUIRE_THROWS_AS(el::err::ParameterFailure, first.withUnprotectedData({}));
    }

    void testInternalRoundTripCopyMoveAndErase() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setProtectedDataMode(ProtectedDataMode::InternalOnly);
        configuration.validateProtectedDataSupport();
        const auto plaintext = bytesFromHex("00010203040506070809aabbccddeeff");
        auto protectedData = ProtectedByteBlock{plaintext};
        REQUIRE_FALSE(protectedData.isEmpty());
        REQUIRE_EQUAL(protectedData.byteLength(), plaintext.length());
        REQUIRE_EQUAL(protectedData.unprotect(), plaintext);
        REQUIRE(protectedData.unprotect().isSensitive());

        auto copied = protectedData;
        auto moved = std::move(copied);
        REQUIRE(copied.isEmpty());
        REQUIRE_EQUAL(moved.unprotect(), plaintext);
        auto callbackCalled = false;
        moved.withUnprotectedData([&](const el::ConstByteSpan data) -> void {
            callbackCalled = true;
            REQUIRE_EQUAL(el::ByteBlock::fromSpan(data), plaintext);
        });
        REQUIRE(callbackCalled);
        moved.secureErase();
        REQUIRE(moved.isEmpty());
    }

    void testCallbackExceptionAndConfigurationLock() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto protectedData = ProtectedByteBlock{bytesFromHex("01020304")};
        {
            const auto observer = EraseObserverGuard{};
            REQUIRE_THROWS_AS(el::err::RuntimeError, protectedData.withUnprotectedData([](el::ConstByteSpan) -> void {
                throw el::err::RuntimeError{"stop"};
            }));
            REQUIRE(_fourByteEraseCount.load() >= 1U);
        }
        REQUIRE_THROWS_AS(el::err::LogicFailure, configuration.setProtectedDataMode(ProtectedDataMode::Automatic));
        REQUIRE_NOTHROW(configuration.setProtectedDataMode(ProtectedDataMode::InternalOnly));
        REQUIRE_NOTHROW(configuration.reset());
        REQUIRE_EQUAL(configuration.protectedDataMode(), ProtectedDataMode::InternalOnly);
    }

    void testApplicationLifetimeIsolation() {
        auto protectedData = ProtectedByteBlock{};
        {
            const auto applicationScope = ApplicationTestScope<>{};
            el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
            protectedData = ProtectedByteBlock{bytesFromHex("aabbccdd")};
        }
        {
            const auto applicationScope = ApplicationTestScope<>{};
            el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
            REQUIRE_THROWS_AS(CryptologyError, protectedData.unprotect());
        }
    }

    void testPlatformSelection() {
#if defined(__linux__)
        {
            const auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = el::core::application().cryptologyConfiguration();
            REQUIRE_EQUAL(configuration.protectedDataMode(), ProtectedDataMode::Automatic);
            REQUIRE_NOTHROW(configuration.validateProtectedDataSupport());
            REQUIRE_EQUAL(ProtectedByteBlock{bytesFromHex("0102")}.unprotect(), bytesFromHex("0102"));
        }
        {
            const auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = el::core::application().cryptologyConfiguration();
            configuration.setProtectedDataMode(ProtectedDataMode::InternalOnly);
            REQUIRE_NOTHROW(configuration.validateProtectedDataSupport());
        }
        {
            const auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = el::core::application().cryptologyConfiguration();
            configuration.setProtectedDataMode(ProtectedDataMode::PlatformOnly);
            REQUIRE_THROWS_AS(CryptologyError, configuration.validateProtectedDataSupport());
            REQUIRE_THROWS_AS(
                el::err::LogicFailure, configuration.setProtectedDataMode(ProtectedDataMode::InternalOnly));
        }
#else
        auto nativeAvailable = false;
        {
            const auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = el::core::application().cryptologyConfiguration();
            configuration.setProtectedDataMode(ProtectedDataMode::PlatformOnly);
            try {
                configuration.validateProtectedDataSupport();
                REQUIRE_EQUAL(ProtectedByteBlock{bytesFromHex("0102")}.unprotect(), bytesFromHex("0102"));
                nativeAvailable = true;
            } catch (const CryptologyError &) {
#if defined(_WIN32)
                REQUIRE(false);
#endif
            }
        }
        if (!nativeAvailable) {
            const auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = el::core::application().cryptologyConfiguration();
            REQUIRE_NOTHROW(configuration.validateProtectedDataSupport());
            REQUIRE_EQUAL(ProtectedByteBlock{bytesFromHex("0102")}.unprotect(), bytesFromHex("0102"));
        }
#endif
    }

    void testAuthenticationFailureAndAccessIsolation() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto plaintext = bytesFromHex("1122334455667788");
        auto protectedData = ProtectedByteBlock{plaintext};

        auto corrupted = protectedData;
        el::cryptology::impl::ProtectedDataTestAccess{corrupted}.corruptEnvelope();
        REQUIRE_THROWS_AS(CryptologyError, corrupted.unprotect());

        auto throwingAccess = ThrowingProtectedDataAccess{};
        const auto accessReset = ProtectedDataAccessReset{};
        el::cryptology::impl::ProtectedDataAccess::setTestAccess(&throwingAccess);
        REQUIRE_NOTHROW([&]() -> void {
            auto first = protectedData;
            auto second = std::move(first);
            REQUIRE(first.isEmpty());
            REQUIRE_FALSE(second.isEmpty());
            REQUIRE_EQUAL(second.byteLength(), plaintext.length());
        }());
        REQUIRE_THROWS_AS(CryptologyError, protectedData.unprotect());
        el::cryptology::impl::ProtectedDataAccess::setTestAccess(nullptr);
        REQUIRE_EQUAL(protectedData.unprotect(), plaintext);
    }

    void testConcurrentEnvelopeUniqueness() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto &configuration = el::core::application().cryptologyConfiguration();
        configuration.setProtectedDataMode(ProtectedDataMode::InternalOnly);
        configuration.validateProtectedDataSupport();
        constexpr auto threadCount = std::size_t{16U};
        auto envelopes = std::vector<el::ByteBlock>{};
        auto envelopesMutex = std::mutex{};
        auto threads = std::vector<std::thread>{};
        threads.reserve(threadCount);
        for (auto index = std::size_t{}; index < threadCount; ++index) {
            threads.emplace_back([&]() -> void {
                auto block = ProtectedByteBlock{bytesFromHex("0102030405060708")};
                const auto lock = std::scoped_lock{envelopesMutex};
                envelopes.emplace_back(el::cryptology::impl::ProtectedDataTestAccess{block}.envelope());
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }
        REQUIRE_EQUAL(envelopes.size(), threadCount);
        for (auto first = std::size_t{}; first < envelopes.size(); ++first) {
            for (auto second = first + 1U; second < envelopes.size(); ++second) {
                REQUIRE_NOT_EQUAL(envelopes[first], envelopes[second]);
            }
        }
    }

    void testInternalKeyErasureAtApplicationDestruction() {
        const auto observer = EraseObserverGuard{};
        {
            const auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = el::core::application().cryptologyConfiguration();
            configuration.setProtectedDataMode(ProtectedDataMode::InternalOnly);
            configuration.validateProtectedDataSupport();
        }
        REQUIRE(_keyEraseCount.load() >= 1U);
    }

    void testInitializationFallbackAndDetectionSuppression() {
        const auto factoryReset = ProtectedDataFactoryReset{};
        ThrowingProtectedDataProvider::creationCount = 0U;
        el::cryptology::impl::ProtectedDataProvider::setTestPlatformFactory(&ThrowingProtectedDataProvider::create);
        {
            const auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = el::core::application().cryptologyConfiguration();
            configuration.setProtectedDataMode(ProtectedDataMode::InternalOnly);
            REQUIRE_NOTHROW(configuration.validateProtectedDataSupport());
            REQUIRE_EQUAL(ThrowingProtectedDataProvider::creationCount.load(), std::size_t{0U});
        }
        {
            const auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = el::core::application().cryptologyConfiguration();
            REQUIRE_NOTHROW(configuration.validateProtectedDataSupport());
            REQUIRE_EQUAL(ThrowingProtectedDataProvider::creationCount.load(), std::size_t{1U});
            REQUIRE_EQUAL(ProtectedByteBlock{bytesFromHex("0102")}.unprotect(), bytesFromHex("0102"));
        }
        {
            const auto applicationScope = ApplicationTestScope<>{};
            auto &configuration = el::core::application().cryptologyConfiguration();
            configuration.setProtectedDataMode(ProtectedDataMode::PlatformOnly);
            REQUIRE_THROWS_AS(CryptologyError, configuration.validateProtectedDataSupport());
            REQUIRE_EQUAL(ThrowingProtectedDataProvider::creationCount.load(), std::size_t{2U});
        }
    }

    void testNonceOverflowFailsClosed() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto provider = el::cryptology::impl::InternalProtectedDataProvider{};
        el::cryptology::impl::ProtectedDataProviderTestAccess{provider}.exhaustNonce();
        const auto plaintext = bytesFromHex("0102");
        REQUIRE_THROWS_AS(CryptologyError, provider.protect(plaintext.span(), plaintext.length()));
    }
};
