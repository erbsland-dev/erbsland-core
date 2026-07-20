// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathParser.hpp"

#include "PathConstants.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/AnyString.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"

#include <utility>

namespace erbsland::path::impl {

using namespace text::literals;
using namespace text;
using namespace unit;

PathParser::PathParser(const String &path, const PathParseMode mode) : _mode{nativeMode(mode)}, _reader{path} {
}

auto PathParser::parse() -> PathDataPtr {
    if (_reader.isAtEnd()) {
        throwParseError("Path text is empty"_el);
    }

    switch (_mode) {
    case PathParseMode::Posix:
        parsePosixPath();
        break;
    case PathParseMode::Windows:
        parseWindowsPath();
        break;
    case PathParseMode::Generic:
        parseGenericPath();
        break;
    case PathParseMode::Native:
    default:
        throwParseError("Invalid path parse mode"_el);
        break;
    }
    return finish();
}

auto PathParser::nativeMode(const PathParseMode mode) noexcept -> PathParseMode {
    if (mode != PathParseMode::Native) {
        return mode;
    }
#ifdef ERBSLAND_OS_WINDOWS
    return PathParseMode::Windows;
#else
    return PathParseMode::Posix;
#endif
}

auto PathParser::isPathSeparator(const Char character, const SeparatorMode mode) noexcept -> bool {
    if (character == U'/') {
        return true;
    }
    return mode == SeparatorMode::SlashAndBackslash && character == U'\\';
}

auto PathParser::normalizedSeparator(const Char character) noexcept -> Char {
    return character == U'\\' ? Char{U'/'} : character;
}

void PathParser::parsePosixPath() {
    _format = PathFormat::Posix;
    parseRootSeparator(SeparatorMode::SlashOnly);
    parseElements(SeparatorMode::SlashOnly);
}

void PathParser::parseWindowsPath() {
    if (parseWindowsRoot()) {
        parseElements(SeparatorMode::SlashAndBackslash);
        return;
    }

    if (parseRootSeparator(SeparatorMode::SlashAndBackslash)) {
        _format = PathFormat::Posix;
        parseElements(SeparatorMode::SlashAndBackslash);
        return;
    }

    _format = PathFormat::Windows;
    parseElements(SeparatorMode::SlashAndBackslash);
}

void PathParser::parseGenericPath() {
    if (equalsNormalized(cWindowsNtPathRoot) || startsWithNormalized(cWindowsNtPathPrefix)) {
        throwParseError("Windows NT paths are not supported"_el);
    }
    if (startsWithNormalized(cDoubleSlash) || startsWithWindowsDriveRoot()) {
        if (!parseWindowsRoot()) {
            throwParseError("Invalid Windows root"_el);
        }
        parseElements(SeparatorMode::SlashAndBackslash);
        return;
    }

    if (parseRootSeparator(SeparatorMode::SlashOnly)) {
        _format = PathFormat::Posix;
        parseElements(SeparatorMode::SlashOnly);
        return;
    }

    _format = PathFormat::Generic;
    parseElements(SeparatorMode::SlashAndBackslash);
}

auto PathParser::parseRootSeparator(const SeparatorMode mode) -> bool {
    const auto checkpoint = save();
    const auto character = readCharacter();
    if (isPathSeparator(character, mode)) {
        _root = cSlash;
        return true;
    }
    restore(checkpoint);
    return false;
}

auto PathParser::parseWindowsRoot() -> bool {
    if (equalsNormalized(cWindowsDevicePathRoot) || startsWithNormalized(cWindowsDevicePathPrefix)) {
        throwParseError("Windows device paths are not supported"_el);
    }
    if (equalsNormalized(cWindowsNtPathRoot) || startsWithNormalized(cWindowsNtPathPrefix)) {
        throwParseError("Windows NT paths are not supported"_el);
    }
    if (startsWithNormalized(cWindowsExtendedPathPrefix)) {
        parseExtendedWindowsRoot();
        return true;
    }
    if (startsWithNormalized(cDoubleSlash)) {
        parseUncRoot();
        return true;
    }
    return parseDriveRoot();
}

void PathParser::parseExtendedWindowsRoot() {
    if (!readNormalizedLiteral(cWindowsExtendedPathPrefix)) {
        throwParseError("Invalid Windows extended path prefix"_el);
    }

    if (startsWithNormalized("UNC/"_el)) {
        if (!readNormalizedLiteral("UNC/"_el)) {
            throwParseError("Invalid Windows extended UNC path prefix"_el);
        }
        parseUncServerAndShare();
        return;
    }

    if (!parseDriveRoot()) {
        throwParseError("Unsupported Windows extended path"_el);
    }
}

auto PathParser::parseDriveRoot() -> bool {
    const auto checkpoint = save();
    const auto drive = readCharacter();
    if (!drive.isAsciiLetter()) {
        restore(checkpoint);
        return false;
    }
    const auto colon = readCharacter();
    if (colon != U':') {
        restore(checkpoint);
        return false;
    }
    const auto separator = readCharacter();
    if (!isPathSeparator(separator, SeparatorMode::SlashAndBackslash)) {
        throwParseError("Windows drive roots must use a separator after the colon"_el);
    }
    _root = String::fromJoined({String::fromCharacter(drive.toAsciiLowercase()), cDriveRootSuffix});
    _format = PathFormat::Windows;
    return true;
}

void PathParser::parseUncRoot() {
    if (!readNormalizedLiteral(cDoubleSlash)) {
        throwParseError("Invalid UNC path prefix"_el);
    }
    parseUncServerAndShare();
}

void PathParser::parseUncServerAndShare() {
    auto server = StringEditor{};
    if (!readRootSegment(server, SeparatorMode::SlashAndBackslash, true)) {
        throwParseError("UNC paths require a server name"_el);
    }
    if (!consumeSeparator(SeparatorMode::SlashAndBackslash)) {
        throwParseError("UNC paths require a share name"_el);
    }

    auto share = StringEditor{};
    if (!readRootSegment(share, SeparatorMode::SlashAndBackslash, false)) {
        throwParseError("UNC paths require a share name"_el);
    }
    const auto hasTrailingSeparator = consumeSeparator(SeparatorMode::SlashAndBackslash);
    (void)hasTrailingSeparator;

    _root = String::fromJoined({cDoubleSlash, server, cSlash, share, cSlash});
    _format = PathFormat::Windows;
}

void PathParser::parseElements(const SeparatorMode mode) {
    _reader.startCapture();
    while (!_reader.isAtEnd()) {
        const auto character = _reader.peek();
        if (isPathSeparator(character, mode)) {
            appendCapturedElement();
            if (!consumeSeparator(mode)) {
                throwParseError("Expected a path separator"_el);
            }
            _reader.startCapture();
            continue;
        }
        const auto read = readCharacter();
        (void)read;
    }
    appendCapturedElement();
}

auto PathParser::readCharacter() -> Char {
    const auto character = _reader.read();
    if (character.isEndOfData()) {
        return character;
    }
    if (_reader.position().toSizeT() > cMaximumPathCharacters.toSizeT()) {
        throwParseError("Path exceeds the maximum character count"_el);
    }
    if (character.isNull()) {
        throwParseError("Path contains a null character"_el);
    }
    if (character.isReplacement()) {
        throwParseError("Path contains invalid encoded text"_el);
    }
    return character;
}

auto PathParser::readNormalizedLiteral(const String &literal) -> bool {
    auto literalReader = StringCharReader{literal};
    while (!literalReader.isAtEnd()) {
        const auto expected = literalReader.read();
        const auto character = readCharacter();
        if (character.isEndOfData() || normalizedSeparator(character) != expected) {
            return false;
        }
    }
    return true;
}

auto PathParser::readRootSegment(StringEditor &builder, const SeparatorMode mode, const bool lowercase) -> bool {
    auto hasText = false;
    while (!_reader.isAtEnd()) {
        const auto character = _reader.peek();
        if (isPathSeparator(character, mode)) {
            break;
        }
        const auto read = readCharacter();
        builder.append(lowercase ? read.toAsciiLowercase() : read);
        hasText = true;
    }
    return hasText;
}

auto PathParser::consumeSeparator(const SeparatorMode mode) -> bool {
    const auto character = readCharacter();
    return isPathSeparator(character, mode);
}

void PathParser::appendCapturedElement() {
    auto element = _reader.takeCapture().toU8String();
    if (element.isEmpty()) {
        return;
    }
    appendElement(std::move(element));
}

void PathParser::appendElement(String element) {
    auto publicElementCount = _elements.count();
    if (!_root.isEmpty()) {
        ++publicElementCount;
    }
    if (publicElementCount >= cMaximumPathElements) {
        throwParseError("Path exceeds the maximum element count"_el);
    }
    _elements.append(std::move(element));
}

auto PathParser::save() const noexcept -> Checkpoint {
    return Checkpoint{_reader.save()};
}

void PathParser::restore(const Checkpoint &checkpoint) noexcept {
    _reader.restore(checkpoint.readerState);
}

void PathParser::throwParseError(String reason) const {
    throw err::ParseError{std::move(reason), _reader.position()};
}

auto PathParser::startsWithNormalized(const String &prefix) -> bool {
    const auto checkpoint = save();
    const auto result = readNormalizedLiteral(prefix);
    restore(checkpoint);
    return result;
}

auto PathParser::equalsNormalized(const String &text) -> bool {
    const auto checkpoint = save();
    const auto result = readNormalizedLiteral(text) && _reader.isAtEnd();
    restore(checkpoint);
    return result;
}

auto PathParser::startsWithWindowsDriveRoot() -> bool {
    const auto checkpoint = save();
    auto result = false;
    const auto drive = readCharacter();
    if (drive.isAsciiLetter() && readCharacter() == U':') {
        result = isPathSeparator(readCharacter(), SeparatorMode::SlashAndBackslash);
    }
    restore(checkpoint);
    return result;
}

auto PathParser::finish() -> PathDataPtr {
    auto result = PathData::create(_format, _root, std::move(_elements));
    if (result.isNull()) {
        throwParseError("Path has no elements"_el);
    }
    return result;
}

}
