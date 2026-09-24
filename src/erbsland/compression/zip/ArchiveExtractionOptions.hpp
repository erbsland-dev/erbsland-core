// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZipProgress.hpp"

#include "../../path/PathCollisionMode.hpp"
#include "../../stream/OutputStreamSettings.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::compression::zip {

/// Options for safely extracting one or more ZIP entries.
/// @tested{ZipArchiveTest}
class ArchiveExtractionOptions final {
public:
    /// Get settings for library-opened output streams.
    auto outputStreamSettings() const noexcept -> const stream::OutputStreamSettings & { return _outputStreamSettings; }
    /// Set settings for library-opened output streams.
    auto setOutputStreamSettings(stream::OutputStreamSettings value) -> ArchiveExtractionOptions & {
        _outputStreamSettings = value;
        return *this;
    }
    /// Get the synchronous entry progress observer.
    auto progress() const noexcept -> const ZipProgressFn & { return _progress; }
    /// Set the synchronous entry progress observer.
    auto setProgress(ZipProgressFn value) -> ArchiveExtractionOptions & {
        _progress = std::move(value);
        return *this;
    }

    /// Get the behavior for an existing destination path.
    [[nodiscard]] auto collisionMode() const noexcept -> path::PathCollisionMode { return _collisionMode; }
    /// Set the behavior for an existing destination path.
    auto setCollisionMode(path::PathCollisionMode value) noexcept -> ArchiveExtractionOptions & {
        _collisionMode = value;
        return *this;
    }
    /// Get whether a valid archive modification time is restored.
    [[nodiscard]] auto preserveModificationTime() const noexcept -> bool { return _preserveModificationTime; }
    /// Set whether a valid archive modification time is restored.
    auto setPreserveModificationTime(bool value) noexcept -> ArchiveExtractionOptions & {
        _preserveModificationTime = value;
        return *this;
    }
    /// Get whether files are committed through sibling temporary files.
    [[nodiscard]] auto useAtomicFiles() const noexcept -> bool { return _useAtomicFiles; }
    /// Set whether files are committed through sibling temporary files.
    auto setUseAtomicFiles(bool value) noexcept -> ArchiveExtractionOptions & {
        _useAtomicFiles = value;
        return *this;
    }
    /// Get the maximum uncompressed length for this extraction.
    [[nodiscard]] auto maximumLength() const noexcept -> unit::ByteLength { return _maximumLength; }
    /// Set the maximum uncompressed length for this extraction.
    auto setMaximumLength(unit::ByteLength value) noexcept -> ArchiveExtractionOptions & {
        _maximumLength = value;
        return *this;
    }

private:
    stream::OutputStreamSettings _outputStreamSettings; ///< Settings for owned streams.
    ZipProgressFn _progress;                            ///< Entry observer.

    path::PathCollisionMode _collisionMode{path::PathCollisionMode::Stop};
    bool _preserveModificationTime{true};
    bool _useAtomicFiles{true};
    unit::ByteLength _maximumLength{unit::ByteLength{256U * 1024U * 1024U}};
};

}
