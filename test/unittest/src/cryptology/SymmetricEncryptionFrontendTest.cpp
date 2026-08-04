// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/impl/symmetric/SymmetricDecryptorBackendAccess.hpp>
#include <erbsland/cryptology/impl/symmetric/SymmetricDecryptorData.hpp>
#include <erbsland/cryptology/impl/symmetric/SymmetricEncryptorBackendAccess.hpp>
#include <erbsland/cryptology/impl/symmetric/SymmetricEncryptorData.hpp>
#include <erbsland/cryptology/symmetric/SymmetricDecryptor.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptor.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <type_traits>
#include <utility>

using namespace el::cryptology;
using namespace el::text::literals;
using el::mem::ByteBlock;
using el::unit::ByteLength;

TESTED_TARGETS(
    CryptologyError SymmetricEncryptor SymmetricDecryptor SymmetricEncryptorData SymmetricDecryptorData
        SymmetricEncryptorBackendAccess SymmetricDecryptorBackendAccess SymmetricDataFactory)
class SymmetricEncryptionFrontendTest final : public el::UnitTest {
private:
    struct FakeState {
        bool erased{false};
        bool failPayload{false};
        std::size_t authenticatedDataCalls{0U};
        std::size_t payloadCalls{0U};
        std::size_t finalizeCalls{0U};
        std::size_t tagCalls{0U};
    };

    class FakeEncryptor final : public el::cryptology::impl::SymmetricEncryptorData {
    public:
        FakeEncryptor(std::shared_ptr<FakeState> state, const SymmetricEncryptionType type) :
            _state{std::move(state)}, _type{type} {}

    public:
        [[nodiscard]] auto type() const noexcept -> SymmetricEncryptionType override { return _type; }

        void addAuthenticatedData(el::mem::ConstByteSpan) override { ++_state->authenticatedDataCalls; }

        [[nodiscard]] auto encrypt(const el::mem::ConstByteSpan data) -> ByteBlock override {
            ++_state->payloadCalls;
            if (_state->failPayload) {
                throw CryptologyError{"Simulated encryption failure."_el};
            }
            return ByteBlock::fromSpan(data);
        }

        [[nodiscard]] auto finalize() -> ByteBlock override {
            ++_state->finalizeCalls;
            return ByteBlock({0xf0U});
        }

        [[nodiscard]] auto tag() const -> SymmetricTag override {
            ++_state->tagCalls;
            return SymmetricTag{ByteBlock{ByteLength{16U}, 0xa5U}};
        }

        void secureErase() noexcept override { _state->erased = true; }

    private:
        std::shared_ptr<FakeState> _state;
        SymmetricEncryptionType _type;
    };

    class FakeDecryptor final : public el::cryptology::impl::SymmetricDecryptorData {
    public:
        FakeDecryptor(std::shared_ptr<FakeState> state, const SymmetricEncryptionType type) :
            _state{std::move(state)}, _type{type} {}

    public:
        [[nodiscard]] auto type() const noexcept -> SymmetricEncryptionType override { return _type; }

        void addAuthenticatedData(el::mem::ConstByteSpan) override { ++_state->authenticatedDataCalls; }

        [[nodiscard]] auto decrypt(const el::mem::ConstByteSpan data) -> ByteBlock override {
            ++_state->payloadCalls;
            if (_state->failPayload) {
                throw CryptologyError{"Simulated decryption failure."_el};
            }
            return ByteBlock::fromSpan(data);
        }

        [[nodiscard]] auto finalize(const SymmetricTag &) -> ByteBlock override {
            ++_state->finalizeCalls;
            return ByteBlock({0xd0U});
        }

        [[nodiscard]] auto finalize() -> ByteBlock override {
            ++_state->finalizeCalls;
            return ByteBlock({0xc0U});
        }

        void secureErase() noexcept override { _state->erased = true; }

    private:
        std::shared_ptr<FakeState> _state;
        SymmetricEncryptionType _type;
    };

    [[nodiscard]] static auto makeEncryptor(
        const std::shared_ptr<FakeState> &state,
        const SymmetricEncryptionType type = SymmetricEncryptionType::Aes256Gcm) -> SymmetricEncryptor {
        auto worker = std::make_unique<FakeEncryptor>(state, type);
        return el::cryptology::impl::SymmetricEncryptorBackendAccess{std::move(worker)}.create();
    }

    [[nodiscard]] static auto makeDecryptor(
        const std::shared_ptr<FakeState> &state,
        const SymmetricEncryptionType type = SymmetricEncryptionType::Aes256Gcm) -> SymmetricDecryptor {
        auto worker = std::make_unique<FakeDecryptor>(state, type);
        return el::cryptology::impl::SymmetricDecryptorBackendAccess{std::move(worker)}.create();
    }

    [[nodiscard]] static auto makeKey() -> SymmetricKey { return SymmetricKey{ByteBlock{ByteLength{32U}, 0x11U}}; }

    [[nodiscard]] static auto makeNonce() -> SymmetricNonce {
        return SymmetricNonce{ByteBlock{ByteLength{12U}, 0x22U}};
    }

    [[nodiscard]] static auto makeIv() -> SymmetricIv { return SymmetricIv{ByteBlock{ByteLength{16U}, 0x33U}}; }

    [[nodiscard]] static auto makeTag() -> SymmetricTag { return SymmetricTag{ByteBlock{ByteLength{16U}, 0x44U}}; }

    static_assert(!std::is_copy_constructible_v<SymmetricEncryptor>);
    static_assert(!std::is_copy_assignable_v<SymmetricEncryptor>);
    static_assert(std::is_nothrow_move_constructible_v<SymmetricEncryptor>);
    static_assert(std::is_nothrow_move_assignable_v<SymmetricEncryptor>);
    static_assert(!std::is_copy_constructible_v<SymmetricDecryptor>);
    static_assert(!std::is_copy_assignable_v<SymmetricDecryptor>);
    static_assert(std::is_nothrow_move_constructible_v<SymmetricDecryptor>);
    static_assert(std::is_nothrow_move_assignable_v<SymmetricDecryptor>);

public:
    void testEmptyAndParameterValidation() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto encryptor = SymmetricEncryptor{};
        auto decryptor = SymmetricDecryptor{};
        REQUIRE(encryptor.isEmpty());
        REQUIRE(decryptor.isEmpty());
        REQUIRE_THROWS_AS(el::err::LogicError, encryptor.type());
        REQUIRE_THROWS_AS(el::err::LogicError, encryptor.encrypt(ByteBlock{}));
        REQUIRE_THROWS_AS(el::err::LogicError, decryptor.decrypt(ByteBlock{}));
        encryptor.secureErase();
        decryptor.secureErase();

        const auto key = makeKey();
        const auto nonce = makeNonce();
        const auto iv = makeIv();
        REQUIRE_THROWS_AS(el::err::ParameterError, SymmetricEncryptor(SymmetricEncryptionType{}, key, nonce));
        REQUIRE_THROWS_AS(
            el::err::ParameterError, SymmetricEncryptor(SymmetricEncryptionType::Aes256Gcm, SymmetricKey{}, nonce));
        REQUIRE_THROWS_AS(
            el::err::ParameterError, SymmetricEncryptor(SymmetricEncryptionType::Aes256CbcRandomFill, key, nonce));
        REQUIRE_THROWS_AS(el::err::ParameterError, SymmetricDecryptor(SymmetricEncryptionType::Aes256Gcm, key, iv));
        REQUIRE_EQUAL(
            SymmetricEncryptor(SymmetricEncryptionType::Aes256Gcm, key, nonce).type(),
            SymmetricEncryptionType::Aes256Gcm);
        REQUIRE_EQUAL(
            SymmetricDecryptor(SymmetricEncryptionType::Aes256CbcRandomFill, key, iv).type(),
            SymmetricEncryptionType::Aes256CbcRandomFill);
        REQUIRE_EQUAL(
            SymmetricEncryptor(SymmetricEncryptionType::ChaCha20Poly1305, key, nonce).type(),
            SymmetricEncryptionType::ChaCha20Poly1305);
    }

    void testAeadEncryptionLifecycleAndSecureErase() {
        const auto state = std::make_shared<FakeState>();
        auto encryptor = makeEncryptor(state);
        REQUIRE_EQUAL(encryptor.type(), SymmetricEncryptionType::Aes256Gcm);
        encryptor.addAuthenticatedData(ByteBlock{});
        REQUIRE(encryptor.encrypt(ByteBlock{}).isEmpty());
        encryptor.addAuthenticatedData(ByteBlock({0xa0U}));
        const auto output = encryptor.encrypt(ByteBlock({0x01U, 0x02U}));
        REQUIRE_EQUAL(output, ByteBlock({0x01U, 0x02U}));
        REQUIRE_EQUAL(state->authenticatedDataCalls, 2U);
        REQUIRE_EQUAL(state->payloadCalls, 2U);
        REQUIRE_THROWS_AS(el::err::LogicError, encryptor.addAuthenticatedData(ByteBlock({0xa1U})));
        REQUIRE_THROWS_AS(el::err::LogicError, encryptor.tag());
        REQUIRE_EQUAL(encryptor.finalize(), ByteBlock({0xf0U}));
        REQUIRE_EQUAL(encryptor.tag().byteLength(), ByteLength{16U});
        REQUIRE_EQUAL(state->finalizeCalls, 1U);
        REQUIRE_EQUAL(state->tagCalls, 1U);
        REQUIRE_THROWS_AS(el::err::LogicError, encryptor.finalize());

        encryptor.secureErase();
        REQUIRE(state->erased);
        REQUIRE(encryptor.isEmpty());
        REQUIRE_THROWS_AS(el::err::LogicError, encryptor.type());
    }

    void testMoveAssignmentSecurelyErasesPreviousWorker() {
        const auto firstState = std::make_shared<FakeState>();
        const auto secondState = std::make_shared<FakeState>();
        auto first = makeEncryptor(firstState);
        auto second = makeEncryptor(secondState);
        second = std::move(first);
        REQUIRE(first.isEmpty());
        REQUIRE(secondState->erased);
        REQUIRE_FALSE(firstState->erased);
        second.secureErase();
        REQUIRE(firstState->erased);
    }

    void testBackendFailureRequiresSecureErase() {
        const auto state = std::make_shared<FakeState>();
        state->failPayload = true;
        auto encryptor = makeEncryptor(state);
        REQUIRE_THROWS_AS(CryptologyError, encryptor.encrypt(ByteBlock({0x01U})));
        REQUIRE_THROWS_AS(el::err::LogicError, encryptor.encrypt(ByteBlock({0x02U})));
        encryptor.secureErase();
        REQUIRE(state->erased);
        REQUIRE(encryptor.isEmpty());
    }

    void testAeadDecryptionLifecycle() {
        const auto state = std::make_shared<FakeState>();
        auto decryptor = makeDecryptor(state);
        decryptor.addAuthenticatedData(ByteBlock({0xa0U}));
        const auto plaintext = decryptor.decrypt(ByteBlock({0x01U, 0x02U}));
        REQUIRE_EQUAL(plaintext, ByteBlock({0x01U, 0x02U}));
        REQUIRE(plaintext.isSensitive());
        REQUIRE_THROWS_AS(el::err::LogicError, decryptor.addAuthenticatedData(ByteBlock({0xa1U})));
        REQUIRE_THROWS_AS(el::err::LogicError, decryptor.finalize());
        REQUIRE_THROWS_AS(el::err::ParameterError, decryptor.finalize(SymmetricTag{}));
        const auto finalPlaintext = decryptor.finalize(makeTag());
        REQUIRE_EQUAL(finalPlaintext, ByteBlock({0xd0U}));
        REQUIRE(finalPlaintext.isSensitive());
        REQUIRE_THROWS_AS(el::err::LogicError, decryptor.finalize(makeTag()));
        decryptor.secureErase();
        REQUIRE(state->erased);
    }

    void testLegacyCbcLifecycle() {
        const auto state = std::make_shared<FakeState>();
        auto decryptor = makeDecryptor(state, SymmetricEncryptionType::Aes256CbcIso9797Method2);
        REQUIRE_THROWS_AS(el::err::LogicError, decryptor.addAuthenticatedData(ByteBlock({0xa0U})));
        REQUIRE_THROWS_AS(el::err::LogicError, decryptor.finalize(makeTag()));
        const auto finalPlaintext = decryptor.finalize();
        REQUIRE_EQUAL(finalPlaintext, ByteBlock({0xc0U}));
        REQUIRE(finalPlaintext.isSensitive());
    }
};
