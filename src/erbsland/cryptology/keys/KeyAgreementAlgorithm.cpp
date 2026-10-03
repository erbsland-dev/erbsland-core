// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyAgreementAlgorithm.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

#include <array>

namespace erbsland::cryptology {

using namespace text;
using namespace unit;
using namespace text::literals;

auto KeyAgreementAlgorithm::publicKeySize() const noexcept -> ByteCount {
    if (_value == P256) {
        return ByteCount{65U};
    }
    return (_value == X25519 || _value == P256) ? ByteCount{32U} : ByteCount::zero();
}

auto KeyAgreementAlgorithm::privateKeySize() const noexcept -> ByteCount {
    return (_value == X25519 || _value == P256) ? ByteCount{32U} : ByteCount::zero();
}

auto KeyAgreementAlgorithm::sharedSecretSize() const noexcept -> ByteCount {
    return (_value == X25519 || _value == P256) ? ByteCount{32U} : ByteCount::zero();
}

auto KeyAgreementAlgorithm::security() const noexcept -> CryptographicSecurity {
    return CryptographicSecurity::Standard;
}

auto KeyAgreementAlgorithm::toString() const -> String {
    if (_value == P256) {
        return "p256"_el;
    }
    return _value == X25519 ? "x25519"_el : String{};
}

auto KeyAgreementAlgorithm::fromString(const String &text) noexcept -> std::optional<KeyAgreementAlgorithm> {
    if (text == "p256"_el) {
        return KeyAgreementAlgorithm{P256};
    }
    if (text == "x25519"_el) {
        return KeyAgreementAlgorithm{X25519};
    }
    return {};
}

auto KeyAgreementAlgorithm::fromStringOrThrow(const String &text) -> KeyAgreementAlgorithm {
    if (const auto result = fromString(text); result.has_value()) {
        return result.value();
    }
    throw err::ParseError{"Unsupported key-agreement algorithm."};
}

auto KeyAgreementAlgorithm::all() noexcept -> std::span<const KeyAgreementAlgorithm> {
    static constexpr auto algorithms =
        std::array<KeyAgreementAlgorithm, 2>{KeyAgreementAlgorithm{X25519}, KeyAgreementAlgorithm{P256}};
    return algorithms;
}

}
