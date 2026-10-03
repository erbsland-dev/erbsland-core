// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathGlobData.hpp"

#include "../PathGlobPattern.hpp"

#include "../../text/StringCharReader.hpp"

namespace erbsland::path::impl {

/// Compile path patterns with one shared reader and the Core component parser.
/// @tested{PathGlobPatternTest FileSourceResolverTest}
class PathGlobCompiler final {
public:
    /// A single-pattern query prepared for a filesystem walk.
    /// @tested{FileSourceResolverTest}
    struct WalkPattern {
        Path prefix;                ///< Literal file or directory prefix, empty for the including directory.
        PathGlobPatternPtr pattern; ///< Remaining glob, null for an entirely literal path.
    };

public:
    /// Bind the source and validate compile flags.
    /// @param source Pattern source in any supported encoding.
    /// @param flags Enabled syntax.
    /// @throws err::ParameterFailure If the flags form an invalid combination.
    PathGlobCompiler(const text::AnyString &source, PathGlobPatternFlags flags);

public: // compilation
    /// Compile all alternatives and publish immutable data.
    /// @return A compiled pattern.
    /// @throws err::ParseError If the pattern is invalid.
    [[nodiscard]] auto compile() -> PathGlobPatternPtr;
    /// Compile one include pattern and extract its literal filesystem prefix.
    /// @param allowWildcards Whether any nonliteral components are allowed.
    /// @return Literal prefix and remaining compiled matcher.
    /// @throws err::ParseError If syntax or navigation after a wildcard is invalid.
    [[nodiscard]] auto compileForWalk(bool allowWildcards) -> WalkPattern;

private:
    /// Parse the complete source, preserving original code-point positions.
    [[nodiscard]] auto parse() -> std::unique_ptr<PathGlobData>;
    /// Parse an alternative and normalize its literal root.
    [[nodiscard]] auto parseAlternative() -> PathGlobData::Alternative;
    /// Parse one component from the shared reader.
    [[nodiscard]] auto parseComponent() -> PathGlobData::Component;
    /// Remove a drive or UNC root from component matching.
    void extractRoot(PathGlobData::Alternative &alternative, bool unc);
    /// Read one LF or CRLF delimiter, rejecting lone CR.
    void consumeNewline();
    /// Throw a syntax error at the current reader position.
    [[noreturn]] void fail(text::String reason) const;

private:
    text::StringCharReader _reader; ///< The single reader over the original source.
    PathGlobPatternFlags _flags;    ///< Enabled compile syntax.
    bool _lineStart{true};          ///< Whether only trimmed whitespace has been read on this physical line.
};

}
