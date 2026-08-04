// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TextStreamSource.hpp"

#include "../../../path/Path.hpp"

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

public: // Access the underlying path.
    /// Get the path from which this source reads data.
    [[nodiscard]] auto filePath() const noexcept -> const path::Path & { return _path; }

protected:
    [[nodiscard]] auto createStream() -> stream::TextInputStreamPtr override;

private:
    path::Path _path;                ///< The path from where this source reads its data.
    SourceIdentifierPtr _identifier; ///< The identifier `file:<path>` for this source.
};

}
