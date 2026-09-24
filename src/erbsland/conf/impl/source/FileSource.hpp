// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TextStreamSource.hpp"

#include "../../../path/Path.hpp"

#include <deque>

namespace erbsland::conf::impl {

/// A file source.
class FileSource final : public TextStreamSource {
public:
    /// Create a new file system source.
    explicit FileSource(path::Path path) noexcept;

    // defaults
    ~FileSource() override = default;

public: // Implement stream source.
    [[nodiscard]] auto identifier() const noexcept -> SourceIdentifierPtr override;
    [[nodiscard]] auto codeSnippet(unit::CodeLocation location) noexcept -> std::optional<text::CodeSnippet> override;

public: // Access the underlying path.
    /// Get the path from which this source reads data.
    [[nodiscard]] auto filePath() const noexcept -> const path::Path & { return _path; }

protected:
    [[nodiscard]] auto createStream() -> stream::TextInputStreamPtr override;
    void rememberLinePosition(unit::LineIndex line, unit::ByteIndex position) noexcept override;

private:
    /// One cached source-line byte position.
    struct LinePosition final {
        unit::LineIndex line;     ///< Zero-based source line.
        unit::ByteIndex position; ///< Byte position at the start of the line.
    };

    /// Find the nearest cached line position at or before the requested line.
    [[nodiscard]] auto linePositionAtOrBefore(unit::LineIndex line) const noexcept -> std::optional<LinePosition>;

private:
    path::Path _path;                              ///< The path from where this source reads its data.
    SourceIdentifierPtr _identifier;               ///< The identifier `file:<path>` for this source.
    std::deque<LinePosition> _recentLinePositions; ///< The last 20 source-line byte positions.
};

}
