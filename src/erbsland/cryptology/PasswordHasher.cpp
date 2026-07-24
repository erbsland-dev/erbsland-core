// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordHasher.hpp"

#include "impl/algorithm/Argon2id.hpp"
#include "impl/algorithm/HmacSha256.hpp"
#include "impl/algorithm/Scrypt.hpp"
#include "impl/PasswordHashData.hpp"
#include "impl/PasswordHashFormat.hpp"

#include "../core/Application.hpp"
#include "../err/ParameterError.hpp"
#include "../mem/ByteArray.hpp"
#include "../random/Random.hpp"
#include "../text/impl/UnsafeU8StringAccess.hpp"
#include "../util/List.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <utility>

namespace erbsland::cryptology {

namespace {
constexpr auto cMaximumPasswordBytes = std::size_t{1024U * 1024U};
}

PasswordHasher::PasswordHasher(PasswordHashKey key, PasswordHashPolicy policy) :
    PasswordHasher{std::optional<PasswordHashKey>{std::move(key)}, {}, std::move(policy)} {
}

PasswordHasher::PasswordHasher(unsafe::UnsafeNoPasswordHashKey, PasswordHashPolicy policy) :
    PasswordHasher{std::nullopt, {}, std::move(policy)} {
}

PasswordHasher::PasswordHasher(
    std::optional<PasswordHashKey> activeKey,
    std::vector<PasswordHashKey> fallbackKeys,
    PasswordHashPolicy policy) noexcept :
    _activeKey{std::move(activeKey)}, _fallbackKeys{std::move(fallbackKeys)}, _policy{std::move(policy)} {
}

auto PasswordHasher::withKeyRotation(
    PasswordHashKey activeKey, util::List<PasswordHashKey> fallbackKeys, PasswordHashPolicy policy) -> PasswordHasher {
    if (!activeKey.isIdentified()) {
        throw err::ParameterError{"The active rotation key must have an identifier", "activeKey"};
    }
    auto resultKeys = std::vector<PasswordHashKey>{};
    auto hasUnnamed = false;
    for (const auto &key : fallbackKeys) {
        if (!key.isIdentified()) {
            if (hasUnnamed) {
                throw err::ParameterError{"Only one unnamed legacy key is allowed", "fallbackKeys"};
            }
            hasUnnamed = true;
        } else {
            if (key.identifier() == activeKey.identifier()) {
                throw err::ParameterError{"Fallback key identifiers must differ from the active key", "fallbackKeys"};
            }
            for (const auto &existing : resultKeys) {
                if (existing.identifier().has_value() && existing.identifier() == key.identifier()) {
                    throw err::ParameterError{"Fallback key identifiers must be unique", "fallbackKeys"};
                }
            }
        }
        resultKeys.push_back(key);
    }
    return PasswordHasher{
        std::optional<PasswordHashKey>{std::move(activeKey)}, std::move(resultKeys), std::move(policy)};
}

auto PasswordHasher::hash(const text::String &password) const -> PasswordHash {
    if (password.length().toSizeT() > cMaximumPasswordBytes) {
        throw err::ParameterError{"Passwords cannot exceed 1 MiB of UTF-8 data", "password"};
    }
    const auto salt = core::application().secureRandom().buildByteBlock(_policy.saltLength());
    return hashWithSalt(password, salt.span());
}

auto PasswordHasher::hashWithSalt(const text::String &password, const mem::ConstByteSpan salt) const -> PasswordHash {
    auto raw = mem::ByteBlock{derive(password, salt, _policy)};
    const auto identifier = _activeKey.has_value() ? _activeKey->identifier() : std::optional<text::String>{};
    const auto header = impl::buildPasswordHashHeader(_policy, _activeKey.has_value(), identifier, salt);
    const auto verifier = protectVerifier(raw, header, _activeKey.has_value() ? &*_activeKey : nullptr);
    const auto record = impl::buildPasswordHashRecord(header, verifier.span());
    return PasswordHash::fromStringOrThrow(record);
}

auto PasswordHasher::derive(
    const text::String &password, const mem::ConstByteSpan salt, const PasswordHashPolicy &policy)
    -> mem::ByteBlockEditor {
    const auto sourceBytes = text::impl::UnsafeU8StringAccess{password}.dataView().dataSpan();
    auto passwordEditor = mem::ByteBlockEditor{unit::ByteLength::fromSizeT(sourceBytes.size())};
    passwordEditor.markAsSensitive();
    passwordEditor.overwrite(mem::toConstByteSpan(sourceBytes));
    const auto passwordBytes = mem::ByteBlock{passwordEditor};
    const auto passwordData = passwordBytes.span();
    if (policy.algorithm() == PasswordHashAlgorithm::Argon2id) {
        const auto argon2id = impl::Argon2id{{
            .memoryKiB = policy.memoryKiB(),
            .passes = policy.passes(),
            .lanes = policy.lanes(),
            .outputLength = policy.outputLength().toSizeT(),
        }};
        return argon2id.derive(passwordData, salt);
    }
    return impl::scrypt(
        passwordData,
        salt,
        policy.scryptCost(),
        policy.scryptBlockSize(),
        policy.scryptParallelization(),
        policy.outputLength().toSizeT());
}

auto PasswordHasher::protectVerifier(const mem::ByteBlock &raw, const text::String &header, const PasswordHashKey *key)
    -> mem::ByteBlockEditor {
    if (key == nullptr) {
        return mem::ByteBlockEditor{raw};
    }
    const auto headerBytes = mem::ByteBlock::fromSpan(text::impl::UnsafeU8StringAccess{header}.dataView().dataSpan());
    const auto separator = mem::ByteArray<1>{mem::Byte{0U}};
    return impl::hmacSha256(
        key->_key.span(),
        {
            headerBytes.span(),
            separator.span(),
            raw.span(),
        });
}

auto PasswordHasher::keyFor(const impl::PasswordHashData &data) const noexcept -> const PasswordHashKey * {
    if (!data.keyed) {
        return nullptr;
    }
    const auto matches = [&](const PasswordHashKey &key) noexcept -> bool {
        return key.identifier() == data.keyIdentifier;
    };
    if (_activeKey.has_value() && matches(*_activeKey)) {
        return &*_activeKey;
    }
    for (const auto &key : _fallbackKeys) {
        if (matches(key)) {
            return &key;
        }
    }
    return nullptr;
}

auto PasswordHasher::needsReplacement(const impl::PasswordHashData &data) const noexcept -> bool {
    if (!_policy.isEqualTo(data.policy)) {
        return true;
    }
    if (_activeKey.has_value() != data.keyed) {
        return true;
    }
    return _activeKey.has_value() && _activeKey->identifier() != data.keyIdentifier;
}

void PasswordHasher::performDummyDerivation(const text::String &password) const {
    auto salt = mem::ByteBlockEditor{_policy.saltLength()};
    salt.markAsSensitive();
    const auto saltBytes = salt.span();
    const auto raw = mem::ByteBlock{derive(password, saltBytes, _policy)};
    const auto header = impl::buildPasswordHashHeader(
        _policy,
        _activeKey.has_value(),
        _activeKey.has_value() ? _activeKey->identifier() : std::optional<text::String>{},
        saltBytes);
    static_cast<void>(protectVerifier(raw, header, _activeKey.has_value() ? &*_activeKey : nullptr));
}

auto PasswordHasher::verify(const text::String &password, const PasswordHash &storedHash) const
    -> PasswordVerification {
    if (password.length().toSizeT() > cMaximumPasswordBytes) {
        const auto empty = text::String{};
        performDummyDerivation(empty);
        return PasswordVerification{false};
    }
    if (!storedHash.isValid()) {
        performDummyDerivation(password);
        return PasswordVerification{false};
    }
    const auto &data = *storedHash._data;
    const auto *key = keyFor(data);
    if (data.keyed && key == nullptr) {
        performDummyDerivation(password);
        return PasswordVerification{false};
    }
    const auto raw = mem::ByteBlock{derive(password, data.salt.span(), data.policy)};
    const auto candidate = mem::ByteBlock{protectVerifier(raw, data.headerThroughSalt, key)};
    const auto expected = mem::ByteBlock{data.verifier};
    if (candidate != expected) {
        return PasswordVerification{false};
    }
    if (needsReplacement(data)) {
        return PasswordVerification{true, hash(password)};
    }
    return PasswordVerification{true};
}

}
