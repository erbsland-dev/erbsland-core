// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathData.hpp"

#include "../../text/String.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor_fwd.hpp"
#include "../../text/StringList.hpp"

#include <cstdint>

namespace erbsland::path::impl {

/// The input format used for parsing a path string.
enum class PathParseMode : uint8_t {
    Generic, ///< Auto-detect roots and otherwise parse a generic relative path.
    Posix,   ///< Parse exactly as POSIX text, where only slash is a separator.
    Windows, ///< Parse as Windows text, where slash and backslash are separators.
    Native,  ///< Parse using the current platform's native path format.
};

/// Stateful parser for path text.
/// @tested{PathConstructionTest}
class PathParser final {
    enum class SeparatorMode : uint8_t {
        SlashOnly,
        SlashAndBackslash,
    };

    struct Checkpoint {
        text::StringCharReaderState readerState;
    };

public:
    /// Create a parser for one path string and parse mode.
    PathParser(const text::String &path, PathParseMode mode);

    /// Parse the configured path.
    [[nodiscard]] auto parse() -> PathDataPtr;

private:
    [[nodiscard]] static auto nativeMode(PathParseMode mode) noexcept -> PathParseMode;
    [[nodiscard]] static auto isPathSeparator(text::Char character, SeparatorMode mode) noexcept -> bool;
    [[nodiscard]] static auto normalizedSeparator(text::Char character) noexcept -> text::Char;

private:
    void parsePosixPath();
    void parseWindowsPath();
    void parseGenericPath();
    auto parseRootSeparator(SeparatorMode mode) -> bool;
    [[nodiscard]] auto parseWindowsRoot() -> bool;
    void parseExtendedWindowsRoot();
    [[nodiscard]] auto parseDriveRoot() -> bool;
    void parseUncRoot();
    void parseUncServerAndShare();
    void parseElements(SeparatorMode mode);

private:
    [[nodiscard]] auto readCharacter() -> text::Char;
    [[nodiscard]] auto readNormalizedLiteral(const text::String &literal) -> bool;
    [[nodiscard]] auto readRootSegment(text::StringEditor &builder, SeparatorMode mode, bool lowercase) -> bool;
    [[nodiscard]] auto consumeSeparator(SeparatorMode mode) -> bool;
    void appendCapturedElement();
    void appendElement(text::String element);
    [[nodiscard]] auto save() const noexcept -> Checkpoint;
    void restore(const Checkpoint &checkpoint) noexcept;
    [[noreturn]] void throwParseError(const text::String &reason) const;
    [[nodiscard]] auto startsWithNormalized(const text::String &prefix) -> bool;
    [[nodiscard]] auto equalsNormalized(const text::String &text) -> bool;
    [[nodiscard]] auto startsWithWindowsDriveRoot() -> bool;
    [[nodiscard]] auto finish() -> PathDataPtr;

private:
    PathParseMode _mode = PathParseMode::Generic;
    text::StringCharReader _reader;
    PathFormat _format = PathFormat::Generic;
    text::String _root;
    text::StringList _elements;
};

}
