// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathData.hpp"
#include "PathParseMode.hpp"

#include "../../text/String.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor_fwd.hpp"
#include "../../text/StringList.hpp"

#include <cstdint>

namespace erbsland::path::impl {

/// Stateful parser for path text.
/// @tested{PathConstructionTest}
class PathParser final {
    /// Selects which path separators the current parser accepts.
    enum class SeparatorMode : uint8_t {
        SlashOnly,
        SlashAndBackslash,
    };

    /// Stores a reader state that can be restored after speculative parsing.
    struct Checkpoint {
        text::StringCharReaderState readerState;
    };

public:
    /// Create a parser for one path string and parse mode.
    PathParser(const text::String &path, PathParseMode mode);

    /// Parse the configured path.
    [[nodiscard]] auto parse() -> PathDataPtr;

private:
    /// Resolve the native parse mode to its platform-specific mode.
    [[nodiscard]] static auto nativeMode(PathParseMode mode) noexcept -> PathParseMode;
    /// Test whether a character is a separator for the selected mode.
    [[nodiscard]] static auto isPathSeparator(text::Char character, SeparatorMode mode) noexcept -> bool;
    /// Convert a Windows separator to its normalized slash form.
    [[nodiscard]] static auto normalizedSeparator(text::Char character) noexcept -> text::Char;

private:
    /// Parse a POSIX-formatted path.
    void parsePosixPath();
    /// Parse a Windows-formatted path.
    void parseWindowsPath();
    /// Parse a path using automatic format detection.
    void parseGenericPath();
    /// Parse an optional root separator.
    auto parseRootSeparator(SeparatorMode mode) -> bool;
    /// Parse a Windows drive, UNC, or extended root.
    [[nodiscard]] auto parseWindowsRoot() -> bool;
    /// Parse a Windows extended-path root.
    void parseExtendedWindowsRoot();
    /// Parse a Windows drive-letter root.
    [[nodiscard]] auto parseDriveRoot() -> bool;
    /// Parse a Windows UNC root.
    void parseUncRoot();
    /// Parse the server and share portions of a UNC root.
    void parseUncServerAndShare();
    /// Parse the elements after a path root.
    void parseElements(SeparatorMode mode);

private:
    /// Read and validate the next character.
    [[nodiscard]] auto readCharacter() -> text::Char;
    /// Read a literal while treating both supported separators as slashes.
    [[nodiscard]] auto readNormalizedLiteral(const text::String &literal) -> bool;
    /// Read one root segment into a builder.
    [[nodiscard]] auto readRootSegment(text::StringEditor &builder, SeparatorMode mode, bool lowercase) -> bool;
    /// Read one separator for the selected mode.
    [[nodiscard]] auto consumeSeparator(SeparatorMode mode) -> bool;
    /// Append the reader's captured path element, if present.
    void appendCapturedElement();
    /// Append one validated path element.
    void appendElement(text::String element);
    /// Save the reader state for speculative parsing.
    [[nodiscard]] auto save() const noexcept -> Checkpoint;
    /// Restore a reader state saved for speculative parsing.
    void restore(const Checkpoint &checkpoint) noexcept;
    /// Throw a parse error at the current reader position.
    [[noreturn]] void throwParseError(text::String reason) const;
    /// Test whether the remaining input starts with a normalized prefix.
    [[nodiscard]] auto startsWithNormalized(const text::String &prefix) -> bool;
    /// Test whether the remaining input equals normalized text.
    [[nodiscard]] auto equalsNormalized(const text::String &text) -> bool;
    /// Test whether the remaining input begins with a Windows drive root.
    [[nodiscard]] auto startsWithWindowsDriveRoot() -> bool;
    /// Build path data from the parsed format, root, and elements.
    [[nodiscard]] auto finish() -> PathDataPtr;

private:
    PathParseMode _mode = PathParseMode::Generic;
    text::StringCharReader _reader;
    PathFormat _format = PathFormat::Generic;
    text::String _root;
    text::StringList _elements;
};

}
