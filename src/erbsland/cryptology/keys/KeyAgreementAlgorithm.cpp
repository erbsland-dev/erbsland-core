// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyAgreementAlgorithm.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

#include <array>

namespace erbsland::cryptology {

using namespace text::literals;

auto KeyAgreementAlgorithm::publicKeySize() const noexcept -> unit::ByteLength {
    return _value == X25519 ? unit::ByteLength{32U} : unit::ByteLength::zero();
}

auto KeyAgreementAlgorithm::privateKeySize() const noexcept -> unit::ByteLength {
    return _value == X25519 ? unit::ByteLength{32U} : unit::ByteLength::zero();
}

auto KeyAgreementAlgorithm::sharedSecretSize() const noexcept -> unit::ByteLength {
    return _value == X25519 ? unit::ByteLength{32U} : unit::ByteLength::zero();
}

auto KeyAgreementAlgorithm::security() const noexcept -> CryptographicSecurity {
    return CryptographicSecurity::Standard;
}

auto KeyAgreementAlgorithm::toString() const -> text::String {
    return _value == X25519 ? "x25519"_el : text::String{};
}

auto KeyAgreementAlgorithm::fromString(const text::String &text) noexcept -> std::optional<KeyAgreementAlgorithm> {
    if (text == "x25519"_el) {
        return KeyAgreementAlgorithm{X25519};
    }
    return {};
}

auto KeyAgreementAlgorithm::fromStringOrThrow(const text::String &text) -> KeyAgreementAlgorithm {
    if (const auto result = fromString(text); result.has_value()) {
        return result.value();
    }
    throw err::ParseError{"Unsupported key-agreement algorithm."};
}

auto KeyAgreementAlgorithm::all() noexcept -> std::span<const KeyAgreementAlgorithm> {
    static constexpr auto algorithms = std::array<KeyAgreementAlgorithm, 1>{KeyAgreementAlgorithm{X25519}};
    return algorithms;
}

}
