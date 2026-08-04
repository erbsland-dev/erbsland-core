// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SymmetricEncryptionType.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

#include <array>

namespace erbsland::cryptology {

using namespace text::literals;

auto SymmetricEncryptionType::isAead() const noexcept -> bool {
    return _value == Aes256Gcm || _value == ChaCha20Poly1305 || _value == Aes128Gcm;
}

auto SymmetricEncryptionType::requiresIv() const noexcept -> bool {
    return _value == Aes256CbcRandomFill || _value == Aes256CbcIso9797Method2;
}

auto SymmetricEncryptionType::cipher() const noexcept -> SymmetricCipher {
    switch (_value) {
    case Aes256Gcm:
    case Aes128Gcm:
    case Aes256CbcRandomFill:
    case Aes256CbcIso9797Method2:
        return SymmetricCipher::Aes;
    case ChaCha20Poly1305:
        return SymmetricCipher::ChaCha20;
    case None:
        return SymmetricCipher::None;
    }
    return SymmetricCipher::None;
}

auto SymmetricEncryptionType::keyBitCount() const noexcept -> std::size_t {
    return keyLength().toSizeT() * 8U;
}

auto SymmetricEncryptionType::keyLength() const noexcept -> unit::ByteLength {
    switch (_value) {
    case Aes128Gcm:
        return unit::ByteLength{16U};
    case Aes256Gcm:
    case ChaCha20Poly1305:
    case Aes256CbcRandomFill:
    case Aes256CbcIso9797Method2:
        return unit::ByteLength{32U};
    case None:
        return unit::ByteLength::zero();
    }
    return unit::ByteLength::zero();
}

auto SymmetricEncryptionType::nonceLength() const noexcept -> unit::ByteLength {
    return isAead() ? unit::ByteLength{12U} : unit::ByteLength::zero();
}

auto SymmetricEncryptionType::tagLength() const noexcept -> unit::ByteLength {
    return isAead() ? unit::ByteLength{16U} : unit::ByteLength::zero();
}

auto SymmetricEncryptionType::ivLength() const noexcept -> unit::ByteLength {
    return requiresIv() ? unit::ByteLength{16U} : unit::ByteLength::zero();
}

auto SymmetricEncryptionType::maximumEncryptedLength(const unit::ByteLength originalLength) const noexcept
    -> unit::ByteLength {
    if (!isValid()) {
        return unit::ByteLength::zero();
    }
    if (!requiresIv() || originalLength.isInfinite()) {
        return originalLength;
    }
    const auto remainder = originalLength % 16U;
    if (_value == Aes256CbcRandomFill && remainder.isZero()) {
        return originalLength;
    }
    return originalLength + (unit::ByteLength{16U} - remainder);
}

auto SymmetricEncryptionType::security() const noexcept -> CryptographicSecurity {
    return _value == Aes128Gcm || _value == None ? CryptographicSecurity::Standard : CryptographicSecurity::High;
}

auto SymmetricEncryptionType::toString() const -> text::String {
    switch (_value) {
    case Aes256Gcm:
        return "aes-256-gcm"_el;
    case ChaCha20Poly1305:
        return "chacha20-poly1305"_el;
    case Aes128Gcm:
        return "aes-128-gcm"_el;
    case Aes256CbcRandomFill:
        return "aes-256-cbc-random-fill"_el;
    case Aes256CbcIso9797Method2:
        return "aes-256-cbc-iso9797-method2"_el;
    case None:
        return {};
    }
    return {};
}

auto SymmetricEncryptionType::fromString(const text::String &text) noexcept -> std::optional<SymmetricEncryptionType> {
    for (const auto type : all()) {
        if (type.toString() == text) {
            return type;
        }
    }
    return std::nullopt;
}

auto SymmetricEncryptionType::fromStringOrThrow(const text::String &text) -> SymmetricEncryptionType {
    if (const auto result = fromString(text); result.has_value()) {
        return result.value();
    }
    throw err::ParseError{"Unsupported symmetric encryption type."_el};
}

auto SymmetricEncryptionType::all() noexcept -> std::span<const SymmetricEncryptionType> {
    static constexpr auto types = std::array<SymmetricEncryptionType, 5>{
        Aes256Gcm,
        ChaCha20Poly1305,
        Aes128Gcm,
        Aes256CbcRandomFill,
        Aes256CbcIso9797Method2,
    };
    return types;
}

}
