// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZipProgress.hpp"

#include "../../stream/InputStreamSettings.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::compression::zip {

/// Security and resource limits used while opening and reading a ZIP archive.
/// @tested{ZipArchiveTest}
class ArchiveReaderOptions final {
public:
    /// Get the maximum complete in-memory item value.
    auto maximumBufferedItemLength() const noexcept -> unit::ByteCount { return _maximumBufferedItemLength; }
    /// Set the complete item buffering limit independently of total output limits.
    auto setMaximumBufferedItemLength(unit::ByteCount value) -> ArchiveReaderOptions & {
        _maximumBufferedItemLength = value;
        return *this;
    }

    /// Get settings for library-opened input streams.
    auto inputStreamSettings() const noexcept -> const stream::InputStreamSettings & { return _inputStreamSettings; }
    /// Set settings for library-opened input streams.
    auto setInputStreamSettings(stream::InputStreamSettings value) -> ArchiveReaderOptions & {
        _inputStreamSettings = value;
        return *this;
    }
    /// Get the synchronous entry progress observer.
    auto progress() const noexcept -> const ZipProgressFn & { return _progress; }
    /// Set the synchronous entry progress observer.
    auto setProgress(ZipProgressFn value) -> ArchiveReaderOptions & {
        _progress = std::move(value);
        return *this;
    }

    inline static constexpr auto cDefaultMaximumCentralDirectoryLength = unit::ByteCount{64U * 1024U * 1024U};
    inline static constexpr auto cDefaultMaximumItemCount = unit::ItemCount{100000U};
    inline static constexpr auto cDefaultMaximumItemLength = unit::ByteCount{256U * 1024U * 1024U};
    inline static constexpr auto cDefaultMaximumCodecWorkspace = unit::ByteCount{64U * 1024U * 1024U};
    inline static constexpr auto cDefaultMaximumBulkExtractionLength = unit::ByteCount{1024U * 1024U * 1024U};

public:
    /// Get the maximum central-directory byte length.
    [[nodiscard]] auto maximumCentralDirectoryLength() const noexcept -> unit::ByteCount {
        return _maximumCentralDirectoryLength;
    }
    /// Set the maximum central-directory byte length.
    auto setMaximumCentralDirectoryLength(unit::ByteCount value) noexcept -> ArchiveReaderOptions & {
        _maximumCentralDirectoryLength = value;
        return *this;
    }
    /// Get the maximum number of entries accepted from the directory.
    [[nodiscard]] auto maximumItemCount() const noexcept -> unit::ItemCount { return _maximumItemCount; }
    /// Set the maximum number of entries accepted from the directory.
    auto setMaximumItemCount(unit::ItemCount value) noexcept -> ArchiveReaderOptions & {
        _maximumItemCount = value;
        return *this;
    }
    /// Get the maximum uncompressed length of one extracted item.
    [[nodiscard]] auto maximumItemLength() const noexcept -> unit::ByteCount { return _maximumItemLength; }
    /// Set the maximum uncompressed length of one extracted item.
    auto setMaximumItemLength(unit::ByteCount value) noexcept -> ArchiveReaderOptions & {
        _maximumItemLength = value;
        return *this;
    }
    /// Get the maximum decoder workspace length.
    [[nodiscard]] auto maximumCodecWorkspace() const noexcept -> unit::ByteCount { return _maximumCodecWorkspace; }
    /// Set the maximum decoder workspace length.
    auto setMaximumCodecWorkspace(unit::ByteCount value) noexcept -> ArchiveReaderOptions & {
        _maximumCodecWorkspace = value;
        return *this;
    }
    /// Get the maximum total declared output for one bulk extraction.
    [[nodiscard]] auto maximumBulkExtractionLength() const noexcept -> unit::ByteCount {
        return _maximumBulkExtractionLength;
    }
    /// Set the maximum total declared output for one bulk extraction.
    auto setMaximumBulkExtractionLength(unit::ByteCount value) noexcept -> ArchiveReaderOptions & {
        _maximumBulkExtractionLength = value;
        return *this;
    }

private:
    unit::ByteCount _maximumBufferedItemLength{256U * 1024U * 1024U}; ///< In-memory output and fallback limit.

    stream::InputStreamSettings _inputStreamSettings;                 ///< Settings for owned streams.
    ZipProgressFn _progress;                                          ///< Entry observer.

    unit::ByteCount _maximumCentralDirectoryLength{cDefaultMaximumCentralDirectoryLength};
    unit::ItemCount _maximumItemCount{cDefaultMaximumItemCount};
    unit::ByteCount _maximumItemLength{cDefaultMaximumItemLength};
    unit::ByteCount _maximumCodecWorkspace{cDefaultMaximumCodecWorkspace};
    unit::ByteCount _maximumBulkExtractionLength{cDefaultMaximumBulkExtractionLength};
};

}
