// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BaseNFormat.hpp"

#include "../AsciiCategory.hpp"
#include "../Literals.hpp"

#include "../../err/ParameterError.hpp"

#include <cstddef>
#include <utility>

namespace erbsland::text::base_n {

using namespace literals;

BaseNFormat::BaseNFormat() : BaseNFormat{base64()} {
}

BaseNFormat::BaseNFormat(U32String alphabet) :
    BaseNFormat{
        std::move(alphabet),
        std::nullopt,
        CharSet::from(AsciiCategory::Whitespace),
        {},
        unit::CpLength{64U},
        U32String{U"\n"_el}} {
}

BaseNFormat::BaseNFormat(
    U32String alphabet,
    const std::optional<Char> padding,
    CharSet whitespace,
    const BaseNFormatFlags flags,
    const unit::CpLength lineLength,
    U32String lineSeparator) :
    _alphabet{std::move(alphabet)},
    _padding{padding},
    _whitespace{std::move(whitespace)},
    _flags{flags},
    _lineLength{lineLength},
    _lineSeparator{std::move(lineSeparator)} {
    validate();
    rebuildAsciiLookup();
}

auto BaseNFormat::setAlphabet(U32String alphabet) -> BaseNFormat & {
    auto candidate = *this;
    candidate._alphabet = std::move(alphabet);
    candidate.validate();
    candidate.rebuildAsciiLookup();
    *this = std::move(candidate);
    return *this;
}

auto BaseNFormat::setPadding(const std::optional<Char> padding) -> BaseNFormat & {
    auto candidate = *this;
    candidate._padding = padding;
    candidate.validate();
    *this = std::move(candidate);
    return *this;
}

auto BaseNFormat::setWhitespace(CharSet whitespace) -> BaseNFormat & {
    auto candidate = *this;
    candidate._whitespace = std::move(whitespace);
    candidate.validate();
    *this = std::move(candidate);
    return *this;
}

auto BaseNFormat::setFlags(const BaseNFormatFlags flags) -> BaseNFormat & {
    auto candidate = *this;
    candidate._flags = flags;
    candidate.validate();
    *this = std::move(candidate);
    return *this;
}

auto BaseNFormat::addFlags(const BaseNFormatFlags flags) -> BaseNFormat & {
    return setFlags(_flags | flags);
}

auto BaseNFormat::clearFlags(const BaseNFormatFlags flags) -> BaseNFormat & {
    auto result = _flags;
    result.clear(flags);
    return setFlags(result);
}

auto BaseNFormat::setLineLength(const unit::CpLength lineLength) -> BaseNFormat & {
    auto candidate = *this;
    candidate._lineLength = lineLength;
    candidate.validate();
    *this = std::move(candidate);
    return *this;
}

auto BaseNFormat::setLineSeparator(U32String lineSeparator) -> BaseNFormat & {
    auto candidate = *this;
    candidate._lineSeparator = std::move(lineSeparator);
    candidate.validate();
    *this = std::move(candidate);
    return *this;
}

auto BaseNFormat::bitsPerCharacter() const noexcept -> uint8_t {
    switch (_alphabet.length().toRawValue()) {
    case 16U:
        return 4U;
    case 32U:
        return 5U;
    default:
        return 6U;
    }
}

auto BaseNFormat::valueFor(const Char character) const noexcept -> std::optional<uint8_t> {
    const auto raw = character.toRawValue();
    if (raw < _asciiLookup.size()) {
        const auto value = _asciiLookup[raw];
        return value < 0 ? std::optional<uint8_t>{} : std::optional<uint8_t>{static_cast<uint8_t>(value)};
    }
    const auto length = _alphabet.length().toSizeT();
    for (std::size_t i = 0; i < length; ++i) {
        if (_alphabet.charAt(unit::CpIndex::fromSizeT(i)) == character) {
            return static_cast<uint8_t>(i);
        }
    }
    return {};
}

auto BaseNFormat::characterFor(const uint8_t value) const noexcept -> Char {
    if (value >= _alphabet.length().toRawValue()) {
        return Char::noCodePoint();
    }
    return _alphabet.charAt(unit::CpIndex{value});
}

auto BaseNFormat::defaultFormat() -> BaseNFormat {
    return base64();
}

auto BaseNFormat::base16() -> BaseNFormat {
    return BaseNFormat{U32String{U"0123456789ABCDEF"_el}};
}

auto BaseNFormat::base32() -> BaseNFormat {
    return BaseNFormat{
        U32String{U"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"_el},
        Char{U'='},
        CharSet::from(AsciiCategory::Whitespace),
        BaseNFormatFlag::EmitPadding | BaseNFormatFlag::RequirePadding,
        unit::CpLength{64U},
        U32String{U"\n"_el}};
}

auto BaseNFormat::base32Hex() -> BaseNFormat {
    return BaseNFormat{
        U32String{U"0123456789ABCDEFGHIJKLMNOPQRSTUV"_el},
        Char{U'='},
        CharSet::from(AsciiCategory::Whitespace),
        BaseNFormatFlag::EmitPadding | BaseNFormatFlag::RequirePadding,
        unit::CpLength{64U},
        U32String{U"\n"_el}};
}

auto BaseNFormat::base64() -> BaseNFormat {
    return BaseNFormat{
        U32String{U"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"_el},
        Char{U'='},
        CharSet::from(AsciiCategory::Whitespace),
        BaseNFormatFlag::EmitPadding | BaseNFormatFlag::RequirePadding,
        unit::CpLength{64U},
        U32String{U"\n"_el}};
}

auto BaseNFormat::base64Url() -> BaseNFormat {
    return BaseNFormat{
        U32String{U"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"_el},
        Char{U'='},
        CharSet::from(AsciiCategory::Whitespace),
        BaseNFormatFlag::EmitPadding | BaseNFormatFlag::RequirePadding,
        unit::CpLength{64U},
        U32String{U"\n"_el}};
}

auto BaseNFormat::base64Pem() -> BaseNFormat {
    return base64().addFlags(BaseNFormatFlag::WrapLines).setLineLength(unit::CpLength{64U});
}

void BaseNFormat::validate() const {
    const auto alphabetLength = _alphabet.length().toRawValue();
    if (alphabetLength != 16U && alphabetLength != 32U && alphabetLength != 64U) {
        throw err::ParameterError{"The alphabet must contain exactly 16, 32, or 64 characters."_el, "alphabet"_el};
    }
    for (std::size_t i = 0; i < alphabetLength; ++i) {
        const auto character = _alphabet.charAt(unit::CpIndex::fromSizeT(i));
        if (!character.isValidUnicode()) {
            throw err::ParameterError{"The alphabet contains an invalid Unicode character."_el, "alphabet"_el};
        }
        if (_whitespace.contains(character)) {
            throw err::ParameterError{"An alphabet character is also accepted as whitespace."_el, "whitespace"_el};
        }
        for (std::size_t j = 0; j < i; ++j) {
            if (_alphabet.charAt(unit::CpIndex::fromSizeT(j)) == character) {
                throw err::ParameterError{"The alphabet contains duplicate characters."_el, "alphabet"_el};
            }
        }
    }
    if (_padding.has_value()) {
        auto paddingInAlphabet = false;
        for (std::size_t i = 0; i < alphabetLength; ++i) {
            paddingInAlphabet = paddingInAlphabet || _alphabet.charAt(unit::CpIndex::fromSizeT(i)) == _padding.value();
        }
        if (!_padding->isValidUnicode() || paddingInAlphabet) {
            throw err::ParameterError{"The padding character is invalid or part of the alphabet."_el, "padding"_el};
        }
        if (_whitespace.contains(_padding.value())) {
            throw err::ParameterError{"The padding character is also accepted as whitespace."_el, "whitespace"_el};
        }
    }
    if ((_flags.contains(BaseNFormatFlag::EmitPadding) || _flags.contains(BaseNFormatFlag::RequirePadding)) &&
        !_padding.has_value()) {
        throw err::ParameterError{"Padding flags require a padding character."_el, "flags"_el};
    }
    if (_flags.contains(BaseNFormatFlag::WrapLines)) {
        if (!_lineLength.isFinite() || _lineLength.isZero()) {
            throw err::ParameterError{"Wrapped lines require a finite non-zero line length."_el, "lineLength"_el};
        }
        if (_lineSeparator.isEmpty()) {
            throw err::ParameterError{"Wrapped lines require a line separator."_el, "lineSeparator"_el};
        }
        auto index = unit::CpIndex{};
        for (std::size_t i = 0; i < _lineSeparator.length().toSizeT(); ++i) {
            const auto character = _lineSeparator.readCharAndAdvance(index);
            if (!_whitespace.contains(character)) {
                throw err::ParameterError{
                    "Every line-separator character must be accepted as whitespace."_el, "lineSeparator"_el};
            }
        }
    }
}

void BaseNFormat::rebuildAsciiLookup() noexcept {
    _asciiLookup.fill(-1);
    const auto length = _alphabet.length().toSizeT();
    for (std::size_t i = 0; i < length; ++i) {
        const auto raw = _alphabet.charAt(unit::CpIndex::fromSizeT(i)).toRawValue();
        if (raw < _asciiLookup.size()) {
            _asciiLookup[raw] = static_cast<int8_t>(i);
        }
    }
}

}
