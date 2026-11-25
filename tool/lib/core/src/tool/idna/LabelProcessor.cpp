// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LabelProcessor.hpp"

#include <erbsland/ParseError.hpp>
#include <erbsland/punycode/PunycodeDecoder.hpp>
#include <erbsland/punycode/PunycodeEncoder.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <algorithm>

namespace tool::idna {

using namespace el::text;
using namespace el::text::literals;

LabelProcessor::LabelProcessor(
    const IdnaData &data, const unicode::NfcNormalizer &normalizer, const unicode::IdnaMapping &mapping) :
    _data{data}, _normalizer{normalizer}, _mapping{mapping} {
}

auto LabelProcessor::toAscii(const el::String &text) const -> el::String {
    const auto prepared = _normalizer.normalize(_mapping.map(text));
    auto split = el::StringSplitter{prepared, U'.'};
    auto labels = el::StringList{};
    while (!split.isAtEnd()) {
        const auto label = prepareLabel(split.next());
        // The Python PSL generator validates each label independently.
        validateBidi(label.codePoints, false);
        labels.append(encodeLabel(label));
    }
    const auto result = labels.join("."_el);
    validateAsciiLength(result, false);
    return result;
}

auto LabelProcessor::prepareLabel(const el::String &label) const -> PreparedLabel {
    if (label.startsWith(cALabelPrefix, el::Char::compareAsciiFolded)) {
        return decodeALabel(label);
    }
    return validateUnicodeLabel(label);
}

auto LabelProcessor::decodeALabel(const el::String &label) const -> PreparedLabel {
    if (!label.startsWith(cALabelPrefix, el::Char::compareAsciiFolded) || label.length() <= cALabelPrefix.length()) {
        throw el::ParseError{"An IDNA A-label must start with xn-- and contain a Punycode payload."_el};
    }
    validateAsciiLength(label, true);
    const auto payload = label.slice(el::StringSide::Back, el::ByteIndex::end(cALabelPrefix.length()));
    const auto decoded = punycode::PunycodeDecoder{payload}.decodeOrThrow();
    const auto normalized = _normalizer.normalize(decoded);
    if (decoded != normalized) {
        throw el::ParseError{"An IDNA A-label decodes to text that is not in NFC."_el};
    }
    auto result = validateUnicodeLabel(decoded);
    if (std::ranges::all_of(result.codePoints, [](const CodePoint &codePoint) { return codePoint.value <= 0x7FU; })) {
        throw el::ParseError{"An IDNA A-label must decode to at least one non-ASCII character."_el};
    }
    auto roundTrip = el::StringEditor{cALabelPrefix};
    roundTrip.append(punycode::PunycodeEncoder{decoded}.encodeOrThrow());
    if (roundTrip.compare(label, el::Char::compareAsciiFolded) != std::strong_ordering::equal) {
        throw el::ParseError{"The IDNA A-label failed its canonical Punycode round trip."_el};
    }
    return result;
}

auto LabelProcessor::validateUnicodeLabel(const el::String &label) const -> PreparedLabel {
    auto values = codePoints(label);
    if (values.empty()) {
        throw el::ParseError{"An IDNA label must not be empty."_el};
    }
    if (values.front().value == U'-' || values.back().value == U'-') {
        throw el::ParseError{"An IDNA label must not begin or end with a hyphen."_el};
    }
    if (values.size() >= 4U && values[2].value == U'-' && values[3].value == U'-') {
        throw el::ParseError{"An IDNA U-label must not use the reserved hyphen form in positions three and four."_el};
    }
    if (_normalizer.isMark(values.front().value)) {
        throw el::ParseError{"An IDNA label must not begin with a combining mark."_el};
    }
    for (auto index = std::size_t{}; index < values.size(); ++index) {
        const auto valueStatus = values[index].attributes.status();
        if (valueStatus == IdnaStatus::Disallowed) {
            throw el::ParseError{"The IDNA label contains a disallowed or unassigned Unicode character."_el};
        }
        if (valueStatus == IdnaStatus::ContextJ || valueStatus == IdnaStatus::ContextO) {
            validateContext(values, index, valueStatus);
        }
    }
    return PreparedLabel{label, std::move(values)};
}

auto LabelProcessor::encodeLabel(const PreparedLabel &label) -> el::String {
    const auto allAscii =
        std::ranges::all_of(label.codePoints, [](const CodePoint &codePoint) { return codePoint.value <= 0x7FU; });
    if (allAscii) {
        validateAsciiLength(label.text, true);
        return label.text;
    }
    auto result = el::StringEditor{cALabelPrefix};
    const auto payload = punycode::PunycodeEncoder{label.text}.encodeOrThrow();
    result.append(payload);
    validateAsciiLength(result, true);
    if (punycode::PunycodeDecoder{payload}.decodeOrThrow() != label.text) {
        throw el::ParseError{"The generated IDNA A-label failed its Unicode round trip."_el};
    }
    return result;
}

void LabelProcessor::validateContext(
    const std::span<const CodePoint> values, const std::size_t index, const IdnaStatus valueStatus) {
    if (valueStatus == IdnaStatus::ContextJ) {
        validateJoiningContext(values, index);
    } else {
        validateOtherContext(values, index);
    }
}

void LabelProcessor::validateJoiningContext(const std::span<const CodePoint> values, const std::size_t index) {
    const auto codePoint = values[index].value;
    if (codePoint == 0x200DU) {
        if (index == 0U || !values[index - 1U].attributes.isVirama()) {
            throw el::ParseError{"U+200D requires a preceding Virama in an IDNA label."_el};
        }
        return;
    }
    if (codePoint != 0x200CU) {
        throw el::ParseError{"The IDNA label contains an unknown CONTEXTJ character."_el};
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
        throw el::ParseError{"U+200C does not satisfy the IDNA joining-context rule."_el};
    }
}

void LabelProcessor::validateOtherContext(const std::span<const CodePoint> values, const std::size_t index) {
    const auto codePoint = values[index].value;
    switch (codePoint) {
    case 0x00B7U:
        if (index == 0U || index + 1U >= values.size() || values[index - 1U].value != U'l' ||
            values[index + 1U].value != U'l') {
            throw el::ParseError{"U+00B7 is permitted only between two lowercase l characters."_el};
        }
        return;
    case 0x0375U:
        if (index + 1U >= values.size() || values[index + 1U].attributes.script() != IdnaScript::Greek) {
            throw el::ParseError{"U+0375 requires a following Greek-script character."_el};
        }
        return;
    case 0x05F3U:
    case 0x05F4U:
        if (index == 0U || values[index - 1U].attributes.script() != IdnaScript::Hebrew) {
            throw el::ParseError{"Hebrew punctuation requires a preceding Hebrew-script character."_el};
        }
        return;
    case 0x30FBU: {
        const auto hasJapaneseScript = std::ranges::any_of(values, [](const CodePoint &value) {
            const auto script = value.attributes.script();
            return value.value != 0x30FBU &&
                (script == IdnaScript::Han || script == IdnaScript::Hiragana || script == IdnaScript::Katakana);
        });
        if (!hasJapaneseScript) {
            throw el::ParseError{"U+30FB requires Hiragana, Katakana, or Han elsewhere in the label."_el};
        }
        return;
    }
    default:
        break;
    }
    if (codePoint >= 0x0660U && codePoint <= 0x0669U) {
        if (std::ranges::any_of(values, [](const CodePoint &value) -> bool {
                return value.value >= 0x06F0U && value.value <= 0x06F9U;
            })) {
            throw el::ParseError{"Arabic-Indic and Extended Arabic-Indic digits must not be mixed."_el};
        }
        return;
    }
    if (codePoint >= 0x06F0U && codePoint <= 0x06F9U) {
        if (std::ranges::any_of(values, [](const CodePoint &value) -> bool {
                return value.value >= 0x0660U && value.value <= 0x0669U;
            })) {
            throw el::ParseError{"Arabic-Indic and Extended Arabic-Indic digits must not be mixed."_el};
        }
        return;
    }
    throw el::ParseError{"The IDNA label contains an unknown CONTEXTO character."_el};
}

void LabelProcessor::validateBidi(const std::span<const CodePoint> values, const bool force) {
    const auto hasRtl = std::ranges::any_of(values, [](const CodePoint &value) -> bool {
        const auto direction = value.attributes.bidi();
        return direction == IdnaBidi::R || direction == IdnaBidi::AL || direction == IdnaBidi::AN;
    });
    if (!hasRtl && !force) {
        return;
    }
    const auto first = values.front().attributes.bidi();
    const auto rtl = first == IdnaBidi::R || first == IdnaBidi::AL;
    if (!rtl && first != IdnaBidi::L) {
        throw el::ParseError{"An IDNA label must begin with bidi class L, R, or AL."_el};
    }
    validateBidiSequence(values, rtl);
}

void LabelProcessor::validateBidiSequence(const std::span<const CodePoint> values, const bool rtl) {
    auto validEnding = false;
    auto numberType = IdnaBidi::Unknown;
    for (const auto codePoint : values) {
        const auto direction = codePoint.attributes.bidi();
        if (direction == IdnaBidi::Unknown) {
            throw el::ParseError{"The IDNA label contains a character with unknown bidi directionality."_el};
        }
        if (!isAllowedBidi(direction, rtl)) {
            if (rtl) {
                throw el::ParseError{"The IDNA label violates the right-to-left bidi character rule."_el};
            }
            throw el::ParseError{"The IDNA label violates the left-to-right bidi character rule."_el};
        }
        if (direction != IdnaBidi::NSM) {
            validEnding = isBidiEnding(direction, rtl);
        }
        if (rtl && (direction == IdnaBidi::EN || direction == IdnaBidi::AN)) {
            if (numberType != IdnaBidi::Unknown && numberType != direction) {
                throw el::ParseError{"An RTL IDNA label must not mix EN and AN digits."_el};
            }
            numberType = direction;
        }
    }
    if (!validEnding) {
        throw el::ParseError{"The IDNA label ends with an illegal bidi directionality."_el};
    }
}

auto LabelProcessor::isAllowedBidi(const IdnaBidi direction, const bool rtl) -> bool {
    switch (direction) {
    case IdnaBidi::EN:
    case IdnaBidi::ES:
    case IdnaBidi::CS:
    case IdnaBidi::ET:
    case IdnaBidi::ON:
    case IdnaBidi::BN:
    case IdnaBidi::NSM:
        return true;
    case IdnaBidi::R:
    case IdnaBidi::AL:
    case IdnaBidi::AN:
        return rtl;
    case IdnaBidi::L:
        return !rtl;
    default:
        return false;
    }
}

auto LabelProcessor::isBidiEnding(const IdnaBidi direction, const bool rtl) -> bool {
    if (direction == IdnaBidi::EN) {
        return true;
    }
    if (rtl) {
        return direction == IdnaBidi::R || direction == IdnaBidi::AL || direction == IdnaBidi::AN;
    }
    return direction == IdnaBidi::L;
}

void LabelProcessor::validateAsciiLength(const el::String &text, const bool label) {
    const auto size = text.length();
    if (label && (size == el::ByteCount{} || size > el::ByteCount{63})) {
        throw el::ParseError{"An IDNA ASCII label must contain between one and 63 bytes."_el};
    }
    if (!label && (size == el::ByteCount{} || size > el::ByteCount{253})) {
        throw el::ParseError{"An IDNA ASCII domain must contain between one and 253 bytes."_el};
    }
}

auto LabelProcessor::codePoints(const el::String &label) const -> std::vector<CodePoint> {
    if (!label.isEncodingValid()) {
        throw el::ParseError{"The IDNA label is not valid UTF-8."_el};
    }
    auto result = std::vector<CodePoint>{};
    result.reserve(label.characterLength().toSizeT());
    for (const auto character : label) {
        const auto value = character.toRawValue();
        result.push_back(CodePoint{value, IdnaAttributes{static_cast<uint16_t>(_data.attributes(value))}});
    }
    return result;
}

}
