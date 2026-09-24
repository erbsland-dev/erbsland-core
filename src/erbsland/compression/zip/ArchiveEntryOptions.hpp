// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompressionMethod.hpp"

#include "../CompressionLevel.hpp"

#include "../../text/String.hpp"
#include "../../time/DateTime.hpp"

#include <optional>
#include <utility>

namespace erbsland::compression::zip {

/// Optional per-entry overrides for an archive writer.
/// @tested{ZipArchiveTest}
class ArchiveEntryOptions final {
public:
    /// Get the optional per-entry compression-method override.
    [[nodiscard]] auto compressionMethod() const noexcept -> const std::optional<CompressionMethod> & {
        return _compressionMethod;
    }
    /// Set the per-entry compression-method override.
    auto setCompressionMethod(CompressionMethod value) noexcept -> ArchiveEntryOptions & {
        _compressionMethod = value;
        return *this;
    }
    /// Get the optional per-entry compression-level override.
    [[nodiscard]] auto compressionLevel() const noexcept -> const std::optional<compression::CompressionLevel> & {
        return _compressionLevel;
    }
    /// Set the per-entry compression-level override.
    auto setCompressionLevel(compression::CompressionLevel value) noexcept -> ArchiveEntryOptions & {
        _compressionLevel = value;
        return *this;
    }
    /// Get the optional per-entry modification-time override.
    [[nodiscard]] auto modificationTime() const noexcept -> const std::optional<time::DateTime> & {
        return _modificationTime;
    }
    /// Set the per-entry modification-time override.
    auto setModificationTime(time::DateTime value) noexcept -> ArchiveEntryOptions & {
        _modificationTime = value;
        return *this;
    }
    /// Get the optional per-entry comment override.
    [[nodiscard]] auto comment() const noexcept -> const std::optional<text::String> & { return _comment; }
    /// Set the per-entry comment override.
    auto setComment(text::String value) noexcept -> ArchiveEntryOptions & {
        _comment = std::move(value);
        return *this;
    }

private:
    std::optional<CompressionMethod> _compressionMethod;
    std::optional<compression::CompressionLevel> _compressionLevel;
    std::optional<time::DateTime> _modificationTime;
    std::optional<text::String> _comment;
};

}
