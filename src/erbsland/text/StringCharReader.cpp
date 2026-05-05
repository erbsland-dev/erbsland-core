// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringCharReader.hpp"

#include "AnyStringView.hpp"

#include "u16/impl/U16StringReader.hpp"
#include "u16/U16String.hpp"
#include "u16/U16StringView.hpp"
#include "u32/impl/U32StringReader.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringView.hpp"
#include "u8/impl/U8StringReader.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringView.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../err/OverflowError.hpp"
#include "../err/ParseNumberError.hpp"
#include "../math/IntegerMath.hpp"

#include <optional>

namespace erbsland::text {

using namespace literals;

StringCharReader::StringCharReader() : _reader{new impl::U8StringReader{U8StringView{}}} {
}

StringCharReader::StringCharReader(const U8String &text) : StringCharReader{U8StringView{text}} {
}

StringCharReader::StringCharReader(const U8StringView &text) : _reader{new impl::U8StringReader{text}} {
}

StringCharReader::StringCharReader(const U16String &text) : StringCharReader{U16StringView{text}} {
}

StringCharReader::StringCharReader(const U16StringView &text) : _reader{new impl::U16StringReader{text}} {
}

StringCharReader::StringCharReader(const U32String &text) : StringCharReader{U32StringView{text}} {
}

StringCharReader::StringCharReader(const U32StringView &text) : _reader{new impl::U32StringReader{text}} {
}

auto StringCharReader::position() const noexcept -> unit::CpIndex {
    return _reader->position();
}

auto StringCharReader::save() const noexcept -> StringCharReaderState {
    return _reader->save();
}

auto StringCharReader::restore(const StringCharReaderState state) noexcept -> bool {
    return _reader->restore(state);
}

void StringCharReader::reset() noexcept {
    _reader->reset();
}

auto StringCharReader::read() noexcept -> Char {
    return _reader->read();
}

auto StringCharReader::readIf(const Char expected) noexcept -> bool {
    return _reader->readIf(expected);
}

auto StringCharReader::readIf(const CharSet &expected) noexcept -> std::optional<Char> {
    return _reader->readIf(expected);
}

auto StringCharReader::readWhile(const ReadFn &readFn, const CharSet &expected, unit::CpLength maximum) noexcept
    -> util::LoopResult {
    return _reader->readWhile(readFn, expected, maximum);
}

auto StringCharReader::readUntil(const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum) noexcept
    -> util::LoopResult {
    return _reader->readUntil(readFn, stopSet, maximum);
}

auto StringCharReader::peek() const noexcept -> Char {
    return _reader->peek();
}

auto StringCharReader::readOrThrow() -> Char {
    return _reader->readOrThrow();
}

auto StringCharReader::readIfOrThrow(const Char expected) -> bool {
    return _reader->readIfOrThrow(expected);
}

auto StringCharReader::readIfOrThrow(const CharSet &expected) -> std::optional<Char> {
    return _reader->readIfOrThrow(expected);
}

auto StringCharReader::peekOrThrow() const -> Char {
    return _reader->peekOrThrow();
}

auto StringCharReader::isAtEnd() const noexcept -> bool {
    return _reader->isAtEnd();
}

auto StringCharReader::canRead(const unit::CpLength count) const noexcept -> bool {
    return _reader->canRead(count);
}

auto StringCharReader::advance() noexcept -> bool {
    return advance(unit::CpLength::one());
}

auto StringCharReader::advance(const unit::CpLength count) noexcept -> bool {
    return _reader->advance(count);
}

auto StringCharReader::advanceIf(const Char expected) noexcept -> bool {
    return _reader->advanceIf(expected);
}

auto StringCharReader::advanceIf(const CharSet &expected) noexcept -> bool {
    return _reader->advanceIf(expected);
}

void StringCharReader::advanceOrThrow() {
    advanceOrThrow(unit::CpLength::one());
}

void StringCharReader::advanceOrThrow(const unit::CpLength count) {
    if (!advance(count)) {
        throw err::OutOfRangeError("String reader cannot advance by the requested number of characters"_el);
    }
}

auto StringCharReader::advanceIfOrThrow(const Char expected) -> bool {
    return _reader->advanceIfOrThrow(expected);
}

auto StringCharReader::advanceIfOrThrow(const CharSet &expected) -> bool {
    return _reader->advanceIfOrThrow(expected);
}

auto StringCharReader::parseInteger(const IntegerParseOptions &options) noexcept -> ReadIntegerResult {
    return scanInteger(options, false);
}

void StringCharReader::startCapture() noexcept {
    _reader->startCapture();
}

auto StringCharReader::takeCapture() noexcept -> AnyStringView {
    return _reader->takeCapture();
}

auto StringCharReader::scanInteger(const IntegerParseOptions &options, const bool strict) -> ReadIntegerResult {
    const auto state = save();
    const auto startPosition = position();
    auto base = IntegerBase{IntegerBase::Decimal};
    auto isNegative = false;
    const auto fail = [this, state, &base](const ReadNumberStatus status) -> ReadIntegerResult {
        const auto errorPosition = position();
        restore(state);
        return ReadIntegerResult{
            .value = {},
            .digitCount = {},
            .isNegative = {},
            .base = base,
            .position = errorPosition,
            .status = status,
        };
    };
    const auto current = [this, strict]() -> Char {
        if (isAtEnd()) {
            return Char::endOfData();
        }
        return strict ? peekOrThrow() : peek();
    };

    if (options.minimumDigits() > options.maximumDigits()) {
        return fail(ReadNumberStatus::ParseError);
    }

    const auto sign = current();
    if (sign == U'-') {
        if (!options.hasFlag(IntegerParseFlag::AcceptMinusSign)) {
            return fail(ReadNumberStatus::ParseError);
        }
        isNegative = true;
        advance();
    } else if (sign == U'+') {
        if (!options.hasFlag(IntegerParseFlag::IgnorePlusSign)) {
            return fail(ReadNumberStatus::ParseError);
        }
        advance();
    }

    const auto prefixState = save();
    if (advanceIf(U'0')) {
        const auto prefix = current();
        const auto prefixedBase =
            prefix.isSignal() ? std::optional<IntegerBase>{} : IntegerBase::fromPrefixChar(prefix);
        if (prefixedBase.has_value()) {
            if (options.hasFixedBase()) {
                return fail(ReadNumberStatus::ParseError);
            }
            base = prefixedBase.value();
            advance();
        } else {
            restore(prefixState);
        }
    }
    if (const auto fixedBase = options.fixedBase(); fixedBase.has_value()) {
        base = fixedBase.value();
    }

    const auto baseValue = static_cast<std::uint64_t>(base.baseFactor());
    const auto allowSeparator = options.hasFlag(IntegerParseFlag::AllowSeparator);
    const auto stopAtMaximum = options.hasFlag(IntegerParseFlag::StopAtMaximum);
    const auto separator = options.separator();
    auto result = std::uint64_t{0U};
    auto digitCount = unit::CpLength::zero();
    auto lastWasSeparator = false;

    while (true) {
        if (digitCount >= options.maximumDigits() && stopAtMaximum) {
            break;
        }

        const auto next = current();
        if (next.isEndOfData()) {
            break;
        }
        if (next.isSignal()) {
            return fail(ReadNumberStatus::ParseError);
        }

        if (allowSeparator && next == separator) {
            if (digitCount.isZero() || lastWasSeparator) {
                return fail(ReadNumberStatus::ParseError);
            }
            advance();
            lastWasSeparator = true;
            continue;
        }

        if (!next.isDigitValue(base)) {
            if (lastWasSeparator) {
                return fail(ReadNumberStatus::ParseError);
            }
            break;
        }
        if (digitCount >= options.maximumDigits()) {
            return fail(ReadNumberStatus::TooManyDigits);
        }

        const auto digit = static_cast<std::uint64_t>(next.digitValue().value());
        if (math::willMultiplyOverflow(result, baseValue)) {
            return fail(ReadNumberStatus::Overflow);
        }
        const auto multipliedResult = static_cast<std::uint64_t>(result * baseValue);
        if (math::willAddOverflow(multipliedResult, digit)) {
            return fail(ReadNumberStatus::Overflow);
        }

        result = static_cast<std::uint64_t>(multipliedResult + digit);
        ++digitCount;
        lastWasSeparator = false;
        advance();
    }

    if (digitCount.isZero()) {
        return fail(ReadNumberStatus::NoDigits);
    }
    if (lastWasSeparator) {
        return fail(ReadNumberStatus::ParseError);
    }
    if (digitCount < options.minimumDigits()) {
        return fail(ReadNumberStatus::TooFewDigits);
    }

    return ReadIntegerResult{
        .value = result,
        .digitCount = digitCount,
        .isNegative = isNegative,
        .base = base,
        .position = startPosition,
        .status = ReadNumberStatus::Success,
    };
}

auto StringCharReader::readIntegerResultOrThrow(const IntegerParseOptions &options) -> ReadIntegerResult {
    const auto state = save();
    try {
        const auto result = scanInteger(options, true);
        if (result.status != ReadNumberStatus::Success) {
            throwError(result.status, result.position);
        }
        return result;
    } catch (const err::Exception &) {
        restore(state);
        throw;
    }
}

void StringCharReader::throwError(const ReadNumberStatus status, const unit::CpIndex position) {
    switch (status) {
    case ReadNumberStatus::Success:
        throw err::ParseNumberError("Integer number was read successfully"_el, status, position);
    case ReadNumberStatus::NoDigits:
        throw err::ParseNumberError("Expected an integer number, got no digits"_el, status, position);
    case ReadNumberStatus::TooFewDigits:
        throw err::ParseNumberError("Expected an integer number with more digits"_el, status, position);
    case ReadNumberStatus::TooManyDigits:
        throw err::ParseNumberError("Integer number has too many digits"_el, status, position);
    case ReadNumberStatus::Overflow:
        throw err::OverflowError("Integer number exceeds the supported range"_el);
    case ReadNumberStatus::ParseError:
        throw err::ParseNumberError("Integer number has invalid syntax"_el, status, position);
    }
    throw err::ParseNumberError("Integer number could not be read"_el, status, position);
}

}
