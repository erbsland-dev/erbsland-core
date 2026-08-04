// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IdnaProcessor.hpp"

#include "PunycodeCodec.hpp"

#include "../../../err/ParseError.hpp"
#include "../../Literals.hpp"
#include "../../NormalizationForm.hpp"
#include "../../StringCharReader.hpp"
#include "../../StringEditor.hpp"
#include "../../UnicodeCategory.hpp"
#include "../../UnicodeCategoryGroup.hpp"

#include <algorithm>

namespace erbsland::text::punycode::impl {

using namespace text::literals;

auto IdnaProcessor::toAscii(const String &text) const -> String {
    const auto prepared = prepareUnicode(text);
    const auto inputLabels = split(prepared);
    auto labels = std::vector<PreparedLabel>{};
    labels.reserve(inputLabels.size());
    for (const auto &label : inputLabels) {
        labels.push_back(prepareLabel(label));
    }
    const auto forceBidi = requiresBidi(labels);
    auto resultLabels = std::vector<String>{};
    resultLabels.reserve(labels.size());
    for (const auto &label : labels) {
        validateBidi(label.codePoints, forceBidi);
        resultLabels.push_back(encodeLabel(label));
    }
    const auto result = join(resultLabels);
    validateAsciiLength(result, false);
    return result;
}

auto IdnaProcessor::toUnicode(const String &text) const -> String {
    if (!text.isValidUtf8()) {
        throw err::ParseError{"The IDNA input is not valid UTF-8."};
    }
    auto reader = StringCharReader{text};
    while (!reader.isAtEnd()) {
        if (!reader.read().isAscii()) {
            throw err::ParseError{"An IDNA A-label or domain must contain ASCII characters only."};
        }
    }
    const auto inputLabels = split(text);
    auto labels = std::vector<PreparedLabel>{};
    labels.reserve(inputLabels.size());
    for (const auto &label : inputLabels) {
        labels.push_back(prepareAsciiLabel(label));
    }
    const auto forceBidi = requiresBidi(labels);
    auto unicodeLabels = std::vector<String>{};
    auto asciiLabels = std::vector<String>{};
    unicodeLabels.reserve(labels.size());
    asciiLabels.reserve(labels.size());
    for (const auto &label : labels) {
        validateBidi(label.codePoints, forceBidi);
        unicodeLabels.push_back(label.text);
        asciiLabels.push_back(encodeLabel(label));
    }
    const auto result = join(unicodeLabels);
    const auto canonicalAscii = join(asciiLabels);
    validateAsciiLength(canonicalAscii, false);
    auto foldedInput = StringEditor{};
    reader.reset();
    while (!reader.isAtEnd()) {
        auto character = reader.read();
        if (character.isAsciiUppercaseLetter()) {
            character = Char{static_cast<char32_t>(character.toRawValue() + (U'a' - U'A'))};
        }
        foldedInput.append(character);
    }
    if (canonicalAscii != String{foldedInput}) {
        throw err::ParseError{"The IDNA domain is not a canonical A-label/NR-LDH representation."};
    }
    return result;
}

auto IdnaProcessor::prepareUnicode(const String &text) -> String {
    if (!text.isValidUtf8()) {
        throw err::ParseError{"The IDNA input is not valid UTF-8."};
    }
    auto result = StringEditor{};
    auto reader = StringCharReader{text};
    while (!reader.isAtEnd()) {
        auto character = reader.read();
        if (!character.isAscii() &&
            (character.isCategory(UnicodeCategory::UppercaseLetter) ||
                character.isCategory(UnicodeCategory::TitlecaseLetter))) {
            throw err::ParseError{"IDNA2008 does not map non-ASCII uppercase or titlecase characters."};
        }
        if (character.isAsciiUppercaseLetter()) {
            character = Char{static_cast<char32_t>(character.toRawValue() + (U'a' - U'A'))};
        }
        result.append(character);
    }
    return result.normalized(NormalizationForm::Nfc);
}

auto IdnaProcessor::split(const String &text) const -> std::vector<String> {
    if (_options.mode() == PunycodeMode::Idna2008Label) {
        if (text.isEmpty()) {
            throw err::ParseError{"An IDNA label must not be empty."};
        }
        return {text};
    }
    auto result = std::vector<String>{};
    auto label = StringEditor{};
    auto reader = StringCharReader{text};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U'.') {
            if (label.isEmpty()) {
                throw err::ParseError{"An IDNA domain must not contain empty labels."};
            }
            result.emplace_back(label);
            label.clear();
        } else {
            label.append(character);
        }
    }
    if (label.isEmpty()) {
        throw err::ParseError{"An IDNA domain must not be empty or end with a dot."};
    }
    result.emplace_back(label);
    return result;
}

auto IdnaProcessor::prepareLabel(const String &label) const -> PreparedLabel {
    if (label.startsWith("xn--"_el, Char::compareAsciiFolded)) {
        return decodeALabel(label);
    }
    return validateUnicodeLabel(label);
}

auto IdnaProcessor::prepareAsciiLabel(const String &label) const -> PreparedLabel {
    auto folded = StringEditor{};
    auto reader = StringCharReader{label};
    while (!reader.isAtEnd()) {
        auto character = reader.read();
        if (!character.isAscii()) {
            throw err::ParseError{"An IDNA label in encoded form must be ASCII."};
        }
        if (character.isAsciiUppercaseLetter()) {
            character = Char{static_cast<char32_t>(character.toRawValue() + (U'a' - U'A'))};
        }
        folded.append(character);
    }
    const auto canonical = String{folded};
    validateAsciiLength(canonical, true);
    if (canonical.startsWith("xn--"_el)) {
        return decodeALabel(canonical);
    }
    return validateUnicodeLabel(canonical);
}

auto IdnaProcessor::decodeALabel(const String &label) const -> PreparedLabel {
    if (!label.startsWith("xn--"_el, Char::compareAsciiFolded) || label.length().toSizeT() <= 4U) {
        throw err::ParseError{"An IDNA A-label must start with xn-- and contain a Punycode payload."};
    }
    validateAsciiLength(label, true);
    const auto payload = label.slice(unit::ByteRange{unit::ByteIndex{4U}, label.length() - unit::ByteLength{4U}});
    const auto decoded = decodePunycodePayload(payload);
    const auto normalized = decoded.normalized(NormalizationForm::Nfc);
    if (decoded != normalized) {
        throw err::ParseError{"An IDNA A-label decodes to text that is not in NFC."};
    }
    auto result = validateUnicodeLabel(decoded);
    if (std::ranges::all_of(result.codePoints, [](const CodePoint &codePoint) { return codePoint.value <= 0x7FU; })) {
        throw err::ParseError{"An IDNA A-label must decode to at least one non-ASCII character."};
    }
    auto roundTrip = StringEditor{"xn--"_el};
    roundTrip.append(encodePunycodePayload(decoded));
    if (roundTrip.compare(label, Char::compareAsciiFolded) != std::strong_ordering::equal) {
        throw err::ParseError{"The IDNA A-label failed its canonical Punycode round trip."};
    }
    return result;
}

auto IdnaProcessor::validateUnicodeLabel(const String &label) const -> PreparedLabel {
    auto values = codePoints(label);
    if (values.empty()) {
        throw err::ParseError{"An IDNA label must not be empty."};
    }
    if (values.front().value == U'-' || values.back().value == U'-') {
        throw err::ParseError{"An IDNA label must not begin or end with a hyphen."};
    }
    if (values.size() >= 4U && values[2].value == U'-' && values[3].value == U'-') {
        throw err::ParseError{"An IDNA U-label must not use the reserved hyphen form in positions three and four."};
    }
    if (Char{values.front().value}.categoryGroup() == UnicodeCategoryGroup::Mark) {
        throw err::ParseError{"An IDNA label must not begin with a combining mark."};
    }
    for (auto index = std::size_t{}; index < values.size(); ++index) {
        const auto valueStatus = values[index].attributes.status();
        if (valueStatus == IdnaStatus::Disallowed) {
            throw err::ParseError{"The IDNA label contains a disallowed or unassigned Unicode character."};
        }
        if (_options.allowedCharacters().has_value() &&
            !_options.allowedCharacters()->contains(Char{values[index].value})) {
            throw err::ParseError{"The IDNA label contains a character rejected by the configured filter."};
        }
        if (valueStatus == IdnaStatus::ContextJ || valueStatus == IdnaStatus::ContextO) {
            validateContext(values, index, valueStatus);
        }
    }
    return PreparedLabel{label, std::move(values)};
}

auto IdnaProcessor::encodeLabel(const PreparedLabel &label) const -> String {
    const auto allAscii =
        std::ranges::all_of(label.codePoints, [](const CodePoint &codePoint) { return codePoint.value <= 0x7FU; });
    if (allAscii) {
        validateAsciiLength(label.text, true);
        return label.text;
    }
    auto result = StringEditor{"xn--"_el};
    const auto payload = encodePunycodePayload(label.text);
    result.append(payload);
    validateAsciiLength(result, true);
    if (decodePunycodePayload(payload) != label.text) {
        throw err::ParseError{"The generated IDNA A-label failed its Unicode round trip."};
    }
    return result;
}

auto IdnaProcessor::requiresBidi(const std::vector<PreparedLabel> &labels) noexcept -> bool {
    return std::ranges::any_of(labels, [](const PreparedLabel &label) {
        return std::ranges::any_of(label.codePoints, [](const CodePoint &codePoint) {
            const auto direction = codePoint.attributes.bidi();
            return direction == IdnaBidi::R || direction == IdnaBidi::AL || direction == IdnaBidi::AN;
        });
    });
}

void IdnaProcessor::validateContext(
    const std::span<const CodePoint> values, const std::size_t index, const IdnaStatus valueStatus) {
    const auto codePoint = values[index].value;
    if (valueStatus == IdnaStatus::ContextJ) {
        if (codePoint == 0x200DU) {
            if (index == 0U || !values[index - 1U].attributes.isVirama()) {
                throw err::ParseError{"U+200D requires a preceding Virama in an IDNA label."};
            }
            return;
        }
        if (codePoint != 0x200CU) {
            throw err::ParseError{"The IDNA label contains an unknown CONTEXTJ character."};
        }
        if (index != 0U && values[index - 1U].attributes.isVirama()) {
            return;
        }
        auto leftValid = false;
        for (auto position = index; position != 0U;) {
            --position;
            const auto type = values[position].attributes.joining();
            if (type == IdnaJoining::T) {
                continue;
            }
            leftValid = type == IdnaJoining::L || type == IdnaJoining::D;
            break;
        }
        auto rightValid = false;
        for (auto position = index + 1U; position < values.size(); ++position) {
            const auto type = values[position].attributes.joining();
            if (type == IdnaJoining::T) {
                continue;
            }
            rightValid = type == IdnaJoining::R || type == IdnaJoining::D;
            break;
        }
        if (!leftValid || !rightValid) {
            throw err::ParseError{"U+200C does not satisfy the IDNA joining-context rule."};
        }
        return;
    }
    if (codePoint == 0x00B7U) {
        if (index == 0U || index + 1U >= values.size() || values[index - 1U].value != U'l' ||
            values[index + 1U].value != U'l') {
            throw err::ParseError{"U+00B7 is permitted only between two lowercase l characters."};
        }
        return;
    }
    if (codePoint == 0x0375U) {
        if (index + 1U >= values.size() || values[index + 1U].attributes.script() != IdnaScript::Greek) {
            throw err::ParseError{"U+0375 requires a following Greek-script character."};
        }
        return;
    }
    if (codePoint == 0x05F3U || codePoint == 0x05F4U) {
        if (index == 0U || values[index - 1U].attributes.script() != IdnaScript::Hebrew) {
            throw err::ParseError{"Hebrew punctuation requires a preceding Hebrew-script character."};
        }
        return;
    }
    if (codePoint == 0x30FBU) {
        const auto hasJapaneseScript = std::ranges::any_of(values, [](const CodePoint &value) {
            const auto script = value.attributes.script();
            return value.value != 0x30FBU &&
                (script == IdnaScript::Han || script == IdnaScript::Hiragana || script == IdnaScript::Katakana);
        });
        if (!hasJapaneseScript) {
            throw err::ParseError{"U+30FB requires Hiragana, Katakana, or Han elsewhere in the label."};
        }
        return;
    }
    if (codePoint >= 0x0660U && codePoint <= 0x0669U) {
        if (std::ranges::any_of(
                values, [](const CodePoint &value) { return value.value >= 0x06F0U && value.value <= 0x06F9U; })) {
            throw err::ParseError{"Arabic-Indic and Extended Arabic-Indic digits must not be mixed."};
        }
        return;
    }
    if (codePoint >= 0x06F0U && codePoint <= 0x06F9U) {
        if (std::ranges::any_of(
                values, [](const CodePoint &value) { return value.value >= 0x0660U && value.value <= 0x0669U; })) {
            throw err::ParseError{"Arabic-Indic and Extended Arabic-Indic digits must not be mixed."};
        }
        return;
    }
    throw err::ParseError{"The IDNA label contains an unknown CONTEXTO character."};
}

void IdnaProcessor::validateBidi(const std::span<const CodePoint> values, const bool force) {
    const auto hasRtl = std::ranges::any_of(values, [](const CodePoint &value) {
        const auto direction = value.attributes.bidi();
        return direction == IdnaBidi::R || direction == IdnaBidi::AL || direction == IdnaBidi::AN;
    });
    if (!hasRtl && !force) {
        return;
    }
    const auto first = values.front().attributes.bidi();
    const auto rtl = first == IdnaBidi::R || first == IdnaBidi::AL;
    if (!rtl && first != IdnaBidi::L) {
        throw err::ParseError{"An IDNA label must begin with bidi class L, R, or AL."};
    }
    auto validEnding = false;
    auto numberType = IdnaBidi::Unknown;
    for (const auto codePoint : values) {
        const auto direction = codePoint.attributes.bidi();
        if (direction == IdnaBidi::Unknown) {
            throw err::ParseError{"The IDNA label contains a character with unknown bidi directionality."};
        }
        if (rtl) {
            const auto allowed = direction == IdnaBidi::R || direction == IdnaBidi::AL || direction == IdnaBidi::AN ||
                direction == IdnaBidi::EN || direction == IdnaBidi::ES || direction == IdnaBidi::CS ||
                direction == IdnaBidi::ET || direction == IdnaBidi::ON || direction == IdnaBidi::BN ||
                direction == IdnaBidi::NSM;
            if (!allowed) {
                throw err::ParseError{"The IDNA label violates the right-to-left bidi character rule."};
            }
            if (direction == IdnaBidi::R || direction == IdnaBidi::AL || direction == IdnaBidi::EN ||
                direction == IdnaBidi::AN) {
                validEnding = true;
            } else if (direction != IdnaBidi::NSM) {
                validEnding = false;
            }
            if (direction == IdnaBidi::EN || direction == IdnaBidi::AN) {
                if (numberType != IdnaBidi::Unknown && numberType != direction) {
                    throw err::ParseError{"An RTL IDNA label must not mix EN and AN digits."};
                }
                numberType = direction;
            }
        } else {
            const auto allowed = direction == IdnaBidi::L || direction == IdnaBidi::EN || direction == IdnaBidi::ES ||
                direction == IdnaBidi::CS || direction == IdnaBidi::ET || direction == IdnaBidi::ON ||
                direction == IdnaBidi::BN || direction == IdnaBidi::NSM;
            if (!allowed) {
                throw err::ParseError{"The IDNA label violates the left-to-right bidi character rule."};
            }
            if (direction == IdnaBidi::L || direction == IdnaBidi::EN) {
                validEnding = true;
            } else if (direction != IdnaBidi::NSM) {
                validEnding = false;
            }
        }
    }
    if (!validEnding) {
        throw err::ParseError{"The IDNA label ends with an illegal bidi directionality."};
    }
}

void IdnaProcessor::validateAsciiLength(const String &text, const bool label) const {
    const auto size = text.length().toSizeT();
    if (label && (size == 0U || size > 63U)) {
        throw err::ParseError{"An IDNA ASCII label must contain between one and 63 bytes."};
    }
    if (!label && _options.mode() == PunycodeMode::Idna2008Domain && (size == 0U || size > 253U)) {
        throw err::ParseError{"An IDNA ASCII domain must contain between one and 253 bytes."};
    }
}

auto IdnaProcessor::join(const std::vector<String> &labels) -> String {
    auto result = StringEditor{};
    for (auto index = std::size_t{}; index < labels.size(); ++index) {
        if (index != 0U) {
            result.append(Char{U'.'});
        }
        result.append(labels[index]);
    }
    return result;
}

auto IdnaProcessor::codePoints(const String &label) -> std::vector<CodePoint> {
    if (!label.isValidUtf8()) {
        throw err::ParseError{"The IDNA label is not valid UTF-8."};
    }
    auto result = std::vector<CodePoint>{};
    result.reserve(label.characterLength().toSizeT());
    auto reader = StringCharReader{label};
    while (!reader.isAtEnd()) {
        const auto value = reader.read().toRawValue();
        result.push_back(CodePoint{value, idnaAttributes(value)});
    }
    return result;
}

}
