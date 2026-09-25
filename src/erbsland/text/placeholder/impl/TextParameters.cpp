// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextParameters.hpp"

#include "../ReplacerError.hpp"

#include "../../../err/Exception.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../Char.hpp"
#include "../../Literals.hpp"
#include "../../named_key/Format.hpp"
#include "../../named_key/Parser.hpp"
#include "../../StringCharReader.hpp"

#include <algorithm>
#include <array>

namespace erbsland::text::placeholder::impl {

using namespace literals;

auto TextParameters::parse(const String &parameter) -> TextParameters {
    static const auto format = named_key::Format{}
                                   .setKeys({
                                       {"start"_el, static_cast<int>(Key::Start)},
                                       {"s"_el, static_cast<int>(Key::Start)},
                                       {"length"_el, static_cast<int>(Key::Length)},
                                       {"l"_el, static_cast<int>(Key::Length)},
                                       {"side"_el, static_cast<int>(Key::Side)},
                                       {"chars"_el, static_cast<int>(Key::Chars)},
                                       {"text"_el, static_cast<int>(Key::Text)},
                                       {"rep"_el, static_cast<int>(Key::Replacement)},
                                       {"format"_el, static_cast<int>(Key::Format)},
                                       {"amount"_el, static_cast<int>(Key::Amount)},
                                       {"contains"_el, static_cast<int>(Key::Contains)},
                                       {"length_eq"_el, static_cast<int>(Key::LengthEqual)},
                                       {"length_gt"_el, static_cast<int>(Key::LengthGreater)},
                                       {"length_lt"_el, static_cast<int>(Key::LengthLess)},
                                       {"empty"_el, static_cast<int>(Key::Empty)},
                                       {"then"_el, static_cast<int>(Key::Then)},
                                       {"else"_el, static_cast<int>(Key::Else)},
                                   })
                                   .setMaximumValues(unit::ItemCount{1U})
                                   .setMaximumValueLength(unit::CpLength::fromSizeT(4000U))
                                   .setValueWithoutKeySeparatorChars(CharSet::from(AsciiCategory::Digit));
    try {
        auto result = TextParameters{};
        auto reader = StringCharReader{parameter};
        auto parser = named_key::Parser{reader, format};
        for (const auto &entry : parser.readAllEntries()) {
            if (entry.isValue()) {
                result.set(Key::Positional, entry.value());
                continue;
            }
            const auto key = static_cast<Key>(entry.keyIndex());
            if (key == Key::Empty) {
                if (!entry.isKey()) {
                    throw ReplacerError{ReplacerErrorCategory::Syntax, "The 'empty' condition takes no value."_el};
                }
                result.set(key, true);
                continue;
            }
            if (!entry.isKeyWithValue()) {
                throw ReplacerError{
                    ReplacerErrorCategory::Syntax, "A placeholder filter parameter value is missing."_el};
            }
            switch (key) {
            case Key::Start:
                result.set(key, parseIndex(entry.value()));
                break;
            case Key::Length:
            case Key::LengthEqual:
            case Key::LengthGreater:
            case Key::LengthLess:
                result.set(key, parseLength(entry.value()));
                break;
            case Key::Side:
                result.set(key, parseSide(entry.value()));
                break;
            case Key::Chars:
                result.set(key, parseChars(entry.value()));
                break;
            case Key::Format:
                result.set(key, parseEscapeFormat(entry.value()));
                break;
            case Key::Amount:
                result.set(key, parseEscapeAmount(entry.value()));
                break;
            case Key::Text:
            case Key::Replacement:
            case Key::Contains:
            case Key::Then:
            case Key::Else:
                result.set(key, entry.value());
                break;
            case Key::Empty:
            case Key::Positional:
                break;
            }
        }
        return result;
    } catch (const ReplacerError &) {
        throw;
    } catch (const err::Exception &error) {
        throw ReplacerError{ReplacerErrorCategory::Syntax, error.reason()};
    }
}

auto TextParameters::hasOnly(const std::initializer_list<Key> allowedKeys) const noexcept -> bool {
    for (const auto &entry : _values) {
        if (std::find(allowedKeys.begin(), allowedKeys.end(), entry.first) == allowedKeys.end()) {
            return false;
        }
    }
    return true;
}

auto TextParameters::conditionCount() const noexcept -> std::size_t {
    static constexpr auto conditionKeys =
        std::array{Key::Contains, Key::LengthEqual, Key::LengthGreater, Key::LengthLess, Key::Empty};
    auto result = std::size_t{};
    for (const auto key : conditionKeys) {
        if (has(key)) {
            ++result;
        }
    }
    return result;
}

auto TextParameters::parseIndex(const String &input) -> unit::CpIndex {
    try {
        return unit::CpIndex::fromSizeTOrThrow(input.toIntegerOrThrow<std::size_t>());
    } catch (const err::Exception &error) {
        throw ReplacerError{ReplacerErrorCategory::Syntax, error.reason()};
    }
}

auto TextParameters::parseLength(const String &input) -> unit::CpLength {
    try {
        return unit::CpLength::fromSizeTOrThrow(input.toIntegerOrThrow<std::size_t>());
    } catch (const err::Exception &error) {
        throw ReplacerError{ReplacerErrorCategory::Syntax, error.reason()};
    }
}

auto TextParameters::parseSide(const String &input) -> Side {
    const auto identifier = input.transformed(Char::toAsciiLowercase);
    if (identifier == "front"_el) {
        return Side::Front;
    }
    if (identifier == "back"_el) {
        return Side::Back;
    }
    if (identifier == "both"_el) {
        return Side::Both;
    }
    throw ReplacerError{ReplacerErrorCategory::Syntax, "The side must be 'both', 'front', or 'back'."_el};
}

auto TextParameters::stringSide(const Side value) noexcept -> std::optional<StringSide> {
    switch (value) {
    case Side::Front:
        return StringSide::Front;
    case Side::Back:
        return StringSide::Back;
    case Side::Both:
        return {};
    }
    return {};
}

auto TextParameters::parseChars(const String &input) -> CharSet {
    const auto inputLength = input.characterLength();
    if (inputLength < unit::CpLength{2U} || !input.startsWith("["_el) || !input.endsWith("]"_el)) {
        throw ReplacerError{ReplacerErrorCategory::Syntax, "A 'chars' value must use the form '[pattern]'."_el};
    }
    return CharSet::fromPattern(input.slice(unit::CpRange{unit::CpIndex::one(), inputLength - unit::CpLength{2U}}));
}

auto TextParameters::parseEscapeFormat(const String &input) -> EscapeFormat {
    const auto identifier = input.transformed(Char::toAsciiLowercase);
    if (const auto result = EscapeFormat::fromString(identifier); result.has_value()) {
        return *result;
    }
    throw ReplacerError{ReplacerErrorCategory::Syntax, "Unknown escape format."_el};
}

auto TextParameters::parseEscapeAmount(const String &input) -> EscapeAmount {
    const auto identifier = input.transformed(Char::toAsciiLowercase);
    if (const auto result = EscapeAmount::fromString(identifier); result.has_value()) {
        return *result;
    }
    throw ReplacerError{ReplacerErrorCategory::Syntax, "Unknown escape amount."_el};
}

void TextParameters::verify(const bool condition, String message) {
    if (!condition) {
        throw ReplacerError{ReplacerErrorCategory::Syntax, std::move(message)};
    }
}

void TextParameters::validateFor(const String &filterName) const {
    if (filterName == "safe"_el) {
        verify(
            hasOnly({Key::Length, Key::Positional}) && count() <= 1U,
            "The 'safe' filter accepts only one length parameter."_el);
        if (const auto value = positional()) {
            [[maybe_unused]] const auto length = parseLength(*value);
        }
        return;
    }
    if (filterName == "trim"_el) {
        verify(hasOnly({Key::Side, Key::Chars}), "The 'trim' filter accepts only 'side' and 'chars'."_el);
        return;
    }
    if (filterName == "slice"_el || filterName == "remove"_el) {
        if (filterName == "remove"_el && (textValue().has_value() || chars().has_value())) {
            verify(count() == 1U, "The remove mode cannot be combined with other parameters."_el);
            return;
        }
        verify(
            hasOnly({Key::Start, Key::Length, Key::Side, Key::Positional}),
            "Unsupported slice or remove parameter."_el);
        auto startValue = start();
        if (const auto value = positional()) {
            verify(!startValue.has_value(), "The start was specified twice."_el);
            startValue = parseIndex(*value);
        }
        if (const auto sideValue = side()) {
            verify(*sideValue != Side::Both, "Side=both is not supported here."_el);
            verify(
                startValue.has_value() != length().has_value(),
                "With 'side', specify exactly one of 'start' or 'length'."_el);
            return;
        }
        verify(startValue.has_value(), "A start or removal mode is required."_el);
        return;
    }
    if (filterName == "replace"_el) {
        verify(
            count() == 2U && textValue().has_value() && replacement().has_value(),
            "The 'replace' filter requires exactly 'text' and 'rep'."_el);
        return;
    }
    if (filterName == "escape"_el) {
        verify(
            hasOnly({Key::Format, Key::Amount}) && format().has_value(),
            "The 'escape' filter requires 'format' and accepts optional 'amount'."_el);
        return;
    }
    if (filterName == "if"_el) {
        verify(
            conditionCount() == 1U && thenText().has_value() && elseText().has_value() &&
                count() == conditionCount() + 2U,
            "The 'if' filter requires one condition, 'then', and 'else'."_el);
        return;
    }
    if (filterName == "error_if"_el) {
        verify(conditionCount() == 1U && count() == 1U, "The 'error_if' filter accepts exactly one condition."_el);
        return;
    }
    throw ReplacerError{ReplacerErrorCategory::Unsupported, "Unknown built-in placeholder filter."_el};
}

auto TextParameters::applySafe(const String &input) const -> String {
    verify(
        hasOnly({Key::Length, Key::Positional}) && count() <= 1U,
        "The 'safe' filter accepts only one length parameter."_el);
    auto resultLength = length().value_or(unit::CpLength{1024U});
    if (const auto positionalValue = positional()) {
        resultLength = parseLength(*positionalValue);
    }
    return input.toSafeString(resultLength);
}

auto TextParameters::applyTrim(const String &input) const -> String {
    verify(hasOnly({Key::Side, Key::Chars}), "The 'trim' filter accepts only 'side' and 'chars'."_el);
    return input.trimmed(chars(), stringSide(side().value_or(Side::Both)));
}

auto TextParameters::applySlice(const String &input) const -> String {
    verify(
        hasOnly({Key::Start, Key::Length, Key::Side, Key::Positional}),
        "Unsupported parameter for the 'slice' filter."_el);
    auto startValue = start();
    if (const auto positionalValue = positional()) {
        verify(!startValue.has_value(), "The slice start was specified twice."_el);
        startValue = parseIndex(*positionalValue);
    }
    const auto lengthValue = length();
    if (const auto sideValue = side()) {
        verify(*sideValue != Side::Both, "The 'slice' filter does not support side=both."_el);
        verify(
            startValue.has_value() != lengthValue.has_value(),
            "With 'side', specify exactly one of 'start' or 'length'."_el);
        const auto convertedSide = *stringSide(*sideValue);
        return lengthValue.has_value() ? input.slice(convertedSide, *lengthValue)
                                       : input.slice(convertedSide, *startValue);
    }
    verify(startValue.has_value(), "The 'slice' filter requires 'start'."_el);
    return input.slice(unit::CpRange{*startValue, lengthValue.value_or(unit::CpLength::infinite())});
}

auto TextParameters::applyRemove(const String &input) const -> String {
    if (const auto text = textValue()) {
        verify(count() == 1U, "The 'text' remove mode cannot be combined."_el);
        return input.removedAll(*text);
    }
    if (const auto characterSet = chars()) {
        verify(count() == 1U, "The 'chars' remove mode cannot be combined."_el);
        return input.removedAll(*characterSet);
    }
    verify(
        hasOnly({Key::Start, Key::Length, Key::Side, Key::Positional}),
        "Unsupported parameter for the 'remove' filter."_el);
    auto startValue = start();
    if (const auto positionalValue = positional()) {
        verify(!startValue.has_value(), "The remove start was specified twice."_el);
        startValue = parseIndex(*positionalValue);
    }
    const auto lengthValue = length();
    if (const auto sideValue = side()) {
        verify(*sideValue != Side::Both, "The 'remove' filter does not support side=both."_el);
        verify(
            startValue.has_value() != lengthValue.has_value(),
            "With 'side', specify exactly one of 'start' or 'length'."_el);
        const auto convertedSide = *stringSide(*sideValue);
        if (lengthValue.has_value()) {
            const auto textLength = input.characterLength();
            const auto removeLength = *lengthValue < textLength ? *lengthValue : textLength;
            const auto removeStart = convertedSide == StringSide::Front
                ? unit::CpIndex::zero()
                : unit::CpIndex::fromSizeT(textLength.toSizeT() - removeLength.toSizeT());
            return input.removed(unit::CpRange{removeStart, removeLength});
        }
        return convertedSide == StringSide::Front ? input.slice(StringSide::Back, *startValue)
                                                  : input.slice(StringSide::Front, *startValue);
    }
    verify(startValue.has_value(), "The 'remove' filter requires a removal mode."_el);
    return input.removed(unit::CpRange{*startValue, lengthValue.value_or(unit::CpLength::infinite())});
}

auto TextParameters::applyReplace(const String &input) const -> String {
    const auto text = textValue();
    const auto replacementText = replacement();
    verify(
        count() == 2U && text.has_value() && replacementText.has_value(),
        "The 'replace' filter requires exactly 'text' and 'rep'."_el);
    return input.replacedAll(*text, *replacementText);
}

auto TextParameters::applyEscape(const String &input) const -> String {
    const auto escapeFormat = format();
    verify(
        hasOnly({Key::Format, Key::Amount}) && escapeFormat.has_value(),
        "The 'escape' filter requires 'format' and accepts optional 'amount'."_el);
    return input.toEscaped(*escapeFormat, amount().value_or(EscapeAmount::Balanced));
}

auto TextParameters::evaluateCondition(const String &input) const -> bool {
    verify(conditionCount() == 1U, "Exactly one condition must be specified."_el);
    if (const auto containsValue = containsText()) {
        return input.contains(*containsValue);
    }
    if (const auto expectedLength = lengthEqual()) {
        return input.characterLength() == *expectedLength;
    }
    if (const auto expectedLength = lengthGreater()) {
        return input.characterLength() > *expectedLength;
    }
    if (const auto expectedLength = lengthLess()) {
        return input.characterLength() < *expectedLength;
    }
    return input.isEmpty();
}

auto TextParameters::applyIf(const String &input) const -> String {
    const auto trueResult = thenText();
    const auto falseResult = elseText();
    verify(
        trueResult.has_value() && falseResult.has_value() && count() == conditionCount() + 2U,
        "The 'if' filter requires one condition, 'then', and 'else'."_el);
    return evaluateCondition(input) ? *trueResult : *falseResult;
}

auto TextParameters::applyErrorIf(const String &input) const -> String {
    verify(count() == conditionCount(), "The 'error_if' filter accepts exactly one condition."_el);
    if (evaluateCondition(input)) {
        throw ReplacerError{ReplacerErrorCategory::Validation, "A placeholder validation condition was met."_el};
    }
    return input;
}

}
