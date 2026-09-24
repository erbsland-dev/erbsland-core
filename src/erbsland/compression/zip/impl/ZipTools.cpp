// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZipTools.hpp"

#include "../../../mem/ByteReader.hpp"
#include "../../../mem/ByteWriter.hpp"
#include "../../../text/CaseSensitivity.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/NormalizationForm.hpp"
#include "../../../text/StringDecoder.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringList.hpp"
#include "../../../text/u8/U8StringConstIterator.hpp"
#include "../../../time/Date.hpp"
#include "../../../time/Time.hpp"
#include "../../../time/TimeAmounts.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/ItemIndex.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace erbsland::compression::zip::impl {

using namespace text::literals;

auto zipTools::normalizeEntryPath(const mem::ByteBlock &name, const bool directory) -> path::Path {
    auto decoded = text::StringDecoder{name}.decode(text::StringEncoding::Utf8);
    auto sanitized = text::StringEditor{};
    for (const auto character : decoded) {
        // Invalid UTF-8 is accepted, but Path reserves U+FFFD as its invalid-input sentinel.
        sanitized.append(character.isReplacement() ? text::Char{U'\ufffc'} : character);
    }
    auto text = text::String{sanitized}.normalized(text::NormalizationForm::Nfc);
    if (text.isEmpty()) {
        throw ZipError{ZipErrorContext{
            ZipErrorReason::UnsafePath,
            ZipOperationPhase::Directory,
            "ZIP item has an empty path"_el,
            "Every file and directory entry must have a non-empty portable relative path."_el}};
    }

    auto hasUnsafeCharacter = false;
    auto characterIndex = std::size_t{};
    const auto driveRoot = text.characterLength().toSizeT() >= 2U && text[unit::CpIndex{0U}].isAsciiLetter() &&
        text[unit::CpIndex{1U}] == U':';
    for (const auto character : text) {
        if (character == U'\\' || character.isControlOrFormat() ||
            (character == U':' && !(driveRoot && characterIndex == 1U))) {
            hasUnsafeCharacter = true;
            break;
        }
        ++characterIndex;
    }
    if (hasUnsafeCharacter) {
        throw ZipError{ZipErrorContext{
            ZipErrorReason::UnsafePath,
            ZipOperationPhase::Directory,
            "ZIP item path is not portable"_el,
            "Backslashes, control characters, format characters, and colons are rejected in archive item paths."_el}};
    }

    // APPNOTE 4.4.17: strip roots before interpreting the stored file name as a portable relative path.
    if (driveRoot) {
        text = text.slice(text::StringSide::Back, unit::CpIndex{2U});
    }
    auto uncRoot = text.startsWith("//"_el);
    while (text.startsWith("/"_el)) {
        text = text.slice(text::StringSide::Back, unit::CpIndex{1U});
    }
    if (directory && text.endsWith("/"_el)) {
        text = text.slice(text::StringSide::Front, unit::CpLength{text.characterLength().toRawValue() - 1U});
    }
    auto parsed = path::Path::fromPosix(text);
    if (parsed.isEmpty()) {
        throw ZipError{ZipErrorContext{
            ZipErrorReason::UnsafePath,
            ZipOperationPhase::Directory,
            "ZIP item path is invalid"_el,
            "The normalized item path cannot be represented as a portable path."_el}};
    }
    auto elements = parsed.elements();
    if (uncRoot) {
        if (elements.count().toRawValue() <= 2U) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::UnsafePath, ZipOperationPhase::Directory, "ZIP item path contains only a UNC root"_el}};
        }
        auto relativeElements = text::StringList{};
        for (auto index = std::size_t{2}; index < elements.count().toSizeT(); ++index) {
            relativeElements.append(elements.getRefOrThrow(unit::ItemIndex::fromSizeT(index)));
        }
        parsed = path::Path::fromElements(relativeElements);
        elements = parsed.elements();
    }
    for (const auto &component : elements) {
        if (!isPortableComponent(component)) {
            throw ZipError{ZipErrorContext{
                ZipErrorReason::UnsafePath,
                ZipOperationPhase::Directory,
                "ZIP item contains an unsafe path component"_el,
                "Dot segments, empty components, Windows-reserved names, and names ending in a dot or space are rejected."_el}
                    .setItemPath(parsed)};
        }
    }
    return parsed;
}

auto zipTools::methodFromRaw(const uint16_t value) -> CompressionMethod {
    switch (value) {
    case 0U:
        return CompressionMethod::Stored;
    case 8U:
        return CompressionMethod::Deflate;
    case 12U:
        return CompressionMethod::Bzip2;
    case 14U:
        return CompressionMethod::Lzma;
    case 93U:
        return CompressionMethod::Zstandard;
    default:
        throw ZipError{ZipErrorContext{
            ZipErrorReason::UnsupportedMethod,
            ZipOperationPhase::Directory,
            "ZIP compression method is not supported"_el,
            "Only Stored, Deflate, Bzip2, LZMA, and Zstandard entries are accepted."_el}};
    }
}

auto zipTools::algorithmForMethod(const CompressionMethod value) -> compression::CompressionAlgorithm {
    switch (value) {
    case CompressionMethod::Deflate:
        return compression::CompressionAlgorithm::Deflate;
    case CompressionMethod::Bzip2:
        return compression::CompressionAlgorithm::Bzip2;
    case CompressionMethod::Lzma:
        return compression::CompressionAlgorithm::Lzma;
    case CompressionMethod::Zstandard:
        return compression::CompressionAlgorithm::Zstandard;
    case CompressionMethod::Stored:
        break;
    }
    throw ZipError{ZipErrorContext{
        ZipErrorReason::UnsupportedMethod, ZipOperationPhase::Compression, "Stored data has no compression codec"_el}};
}

auto zipTools::minimumVersion(const CompressionMethod value, const bool zip64) noexcept -> uint16_t {
    auto methodVersion = uint16_t{};
    switch (value) {
    case CompressionMethod::Stored:
        methodVersion = 10U;
        break;
    case CompressionMethod::Deflate:
        methodVersion = 20U;
        break;
    case CompressionMethod::Bzip2:
        methodVersion = 46U;
        break;
    case CompressionMethod::Lzma:
    case CompressionMethod::Zstandard:
        methodVersion = 63U;
        break;
    }
    return std::max<uint16_t>(methodVersion, zip64 ? uint16_t{45U} : uint16_t{});
}

auto zipTools::pathsConflict(
    const path::Path &left, const bool leftDirectory, const path::Path &right, const bool rightDirectory) noexcept
    -> bool {
    if (left == right) {
        return true;
    }
    if (isPathPrefix(left, right)) {
        return !leftDirectory;
    }
    if (isPathPrefix(right, left)) {
        return !rightDirectory;
    }
    return false;
}

auto zipTools::dateTimeFromDos(const uint16_t date, const uint16_t timeValue) noexcept -> time::DateTime {
    const auto year = static_cast<uint16_t>(1980U + ((date >> 9U) & 0x7fU));
    const auto month = static_cast<uint8_t>((date >> 5U) & 0x0fU);
    const auto day = static_cast<uint8_t>(date & 0x1fU);
    const auto hour = static_cast<uint8_t>((timeValue >> 11U) & 0x1fU);
    const auto minute = static_cast<uint8_t>((timeValue >> 5U) & 0x3fU);
    const auto second = static_cast<uint8_t>((timeValue & 0x1fU) * 2U);
    return time::DateTime{
        time::Date{time::Year{year}, time::Month{month}, time::Day{day}},
        time::Time{time::Hour{hour}, time::Minute{minute}, time::Second{second}}};
}

void zipTools::dateTimeToDos(const time::DateTime &dateTime, uint16_t &date, uint16_t &timeValue) noexcept {
    const auto value = dateTime.isValid() ? dateTime : time::DateTime::now();
    const auto year = std::clamp<uint16_t>(
        static_cast<uint16_t>(value.utcDate().year().toRawValue()), uint16_t{1980U}, uint16_t{2107U});
    const auto month = static_cast<uint16_t>(value.utcDate().month().toRawValue());
    const auto day = static_cast<uint16_t>(value.utcDate().day().toRawValue());
    const auto hour = static_cast<uint16_t>(value.utcTime().hour().toRawValue());
    const auto minute = static_cast<uint16_t>(value.utcTime().minute().toRawValue());
    const auto second = static_cast<uint16_t>(value.utcTime().second().toRawValue());
    date = static_cast<uint16_t>((static_cast<uint16_t>(year - 1980U) << 9U) | (month << 5U) | day);
    const auto dosTime = (static_cast<uint32_t>(hour) << 11U) | (static_cast<uint32_t>(minute) << 5U) |
        (static_cast<uint32_t>(second) / 2U);
    timeValue = static_cast<uint16_t>(dosTime);
}

auto zipTools::modificationTimeFromExtra(
    const mem::ByteBlock &extra, const time::DateTime fallback, mem::ByteWriter *opaque) -> time::DateTime {
    auto result = fallback;
    auto reader = mem::ByteReader{extra};
    while (!reader.isAtEnd()) {
        if (!reader.canRead(4U)) {
            throwMalformed("An extra field header is truncated."_el);
        }
        const auto id = reader.readUInt16OrThrow();
        const auto length = reader.readUInt16OrThrow();
        if (!reader.canRead(unit::ByteLength{length})) {
            throwMalformed("An extra field payload is truncated."_el);
        }
        const auto field = reader.readBytesOrThrow(unit::ByteLength{length});
        if (id == cExtendedTimestampExtraId) {
            // APPNOTE 4.5.5: bit zero announces the four-byte UTC modification time.
            if (length < 1U) {
                throwMalformed("The Extended Timestamp extra field has no flags byte."_el);
            }
            auto fieldReader = mem::ByteReader{field};
            const auto flags = fieldReader.readUInt8OrThrow();
            if ((flags & 0x01U) != 0U) {
                if (length < 5U) {
                    throwMalformed("The Extended Timestamp modification time is truncated."_el);
                }
                result = time::DateTime::fromTimeT(static_cast<std::time_t>(fieldReader.readUInt32OrThrow()));
            }
        } else if (
            id != cZip64ExtraId && id != cUnicodeCommentExtraId && id != cUnicodePathExtraId && opaque != nullptr) {
            opaque->writeUInt16(id).writeUInt16(length).writeBytes(field);
        }
    }
    return result;
}

void zipTools::appendExtendedTimestamp(mem::ByteWriter &extra, const time::DateTime &dateTime) {
    const auto value = dateTime.isValid() ? dateTime : time::DateTime::now();
    const auto timeT = value.toTimeT();
    if (timeT < 0 || static_cast<uint64_t>(timeT) > std::numeric_limits<uint32_t>::max()) {
        return;
    }
    extra.writeUInt16(cExtendedTimestampExtraId).writeUInt16(5U).writeByte(mem::Byte{0x01U});
    extra.writeUInt32(static_cast<uint32_t>(timeT));
}

auto zipTools::globMatches(const text::String &pattern, const text::String &value) noexcept -> bool {
    return globMatchesAt(pattern, 0U, value, 0U);
}

auto zipTools::globMatchesAt(
    const text::String &pattern,
    const std::size_t patternIndex,
    const text::String &value,
    const std::size_t valueIndex) noexcept -> bool {
    const auto patternLength = pattern.characterLength().toSizeT();
    const auto valueLength = value.characterLength().toSizeT();
    if (patternIndex == patternLength) {
        return valueIndex == valueLength;
    }
    const auto token = pattern[unit::CpIndex::fromSizeT(patternIndex)];
    if (token == U'*') {
        const auto crossesSeparators =
            patternIndex + 1U < patternLength && pattern[unit::CpIndex::fromSizeT(patternIndex + 1U)] == U'*';
        const auto nextPattern = patternIndex + (crossesSeparators ? 2U : 1U);
        for (auto nextValue = valueIndex; nextValue <= valueLength; ++nextValue) {
            if (globMatchesAt(pattern, nextPattern, value, nextValue)) {
                return true;
            }
            if (!crossesSeparators && nextValue < valueLength && value[unit::CpIndex::fromSizeT(nextValue)] == U'/') {
                break;
            }
        }
        return false;
    }
    if (valueIndex == valueLength) {
        return false;
    }
    const auto valueCharacter = value[unit::CpIndex::fromSizeT(valueIndex)];
    if (token == U'?' && valueCharacter != U'/') {
        return globMatchesAt(pattern, patternIndex + 1U, value, valueIndex + 1U);
    }
    return token == valueCharacter && globMatchesAt(pattern, patternIndex + 1U, value, valueIndex + 1U);
}

auto zipTools::isPortableComponent(const text::String &component) noexcept -> bool {
    if (component.isEmpty() || component == "."_el || component == ".."_el || component.endsWith("."_el) ||
        component.endsWith(" "_el)) {
        return false;
    }
    constexpr auto reserved = std::array{
        "CON"_el,
        "PRN"_el,
        "AUX"_el,
        "NUL"_el,
        "COM1"_el,
        "COM2"_el,
        "COM3"_el,
        "COM4"_el,
        "COM5"_el,
        "COM6"_el,
        "COM7"_el,
        "COM8"_el,
        "COM9"_el,
        "LPT1"_el,
        "LPT2"_el,
        "LPT3"_el,
        "LPT4"_el,
        "LPT5"_el,
        "LPT6"_el,
        "LPT7"_el,
        "LPT8"_el,
        "LPT9"_el};
    const auto stem = path::Path::fromPosix(component).stem();
    for (const auto &name : reserved) {
        if (stem.compare(name, text::cCaseInsensitive.asciiComparisonFn()) == std::strong_ordering::equal) {
            return false;
        }
    }
    return true;
}

auto zipTools::isPathPrefix(const path::Path &prefix, const path::Path &pathValue) noexcept -> bool {
    const auto prefixElements = prefix.elements();
    const auto pathElements = pathValue.elements();
    if (prefixElements.count() >= pathElements.count()) {
        return false;
    }
    for (auto index = std::size_t{}; index < prefixElements.count().toSizeT(); ++index) {
        const auto itemIndex = unit::ItemIndex::fromSizeT(index);
        if (prefixElements.getRef(itemIndex) != pathElements.getRef(itemIndex)) {
            return false;
        }
    }
    return true;
}

void zipTools::throwMalformed(text::String description) {
    throw ZipError{ZipErrorContext{
        ZipErrorReason::MalformedArchive,
        ZipOperationPhase::Directory,
        "ZIP archive is malformed"_el,
        std::move(description)}};
}

}
