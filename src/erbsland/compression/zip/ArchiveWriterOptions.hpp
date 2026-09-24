// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Zip64Policy.hpp"
#include "ZipProgress.hpp"

#include "../../stream/InputStreamSettings.hpp"
#include "../../stream/OutputStreamSettings.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::compression::zip {

/// Security and resource limits used while creating a ZIP archive.
/// @tested{ZipArchiveTest}
class ArchiveWriterOptions final {
public:
    /// Get settings for library-opened input streams.
    auto inputStreamSettings() const noexcept -> const stream::InputStreamSettings & { return _inputStreamSettings; }
    /// Set settings for library-opened input streams.
    auto setInputStreamSettings(stream::InputStreamSettings value) -> ArchiveWriterOptions & {
        _inputStreamSettings = value;
        return *this;
    }

    /// Get settings for library-opened output streams.
    auto outputStreamSettings() const noexcept -> const stream::OutputStreamSettings & { return _outputStreamSettings; }
    /// Set settings for library-opened output streams.
    auto setOutputStreamSettings(stream::OutputStreamSettings value) -> ArchiveWriterOptions & {
        _outputStreamSettings = value;
        return *this;
    }
    /// Get the synchronous entry progress observer.
    auto progress() const noexcept -> const ZipProgressFn & { return _progress; }
    /// Set the synchronous entry progress observer.
    auto setProgress(ZipProgressFn value) -> ArchiveWriterOptions & {
        _progress = std::move(value);
        return *this;
    }

    /// Get the policy controlling classic ZIP and ZIP64 record emission.
    [[nodiscard]] auto zip64Policy() const noexcept -> Zip64Policy { return _zip64Policy; }
    /// Set the policy controlling classic ZIP and ZIP64 record emission.
    auto setZip64Policy(Zip64Policy value) noexcept -> ArchiveWriterOptions & {
        _zip64Policy = value;
        return *this;
    }
    /// Get the maximum bytes buffered for one uncompressed or compressed entry.
    [[nodiscard]] auto maximumBufferedItemLength() const noexcept -> unit::ByteLength {
        return _maximumBufferedItemLength;
    }
    /// Set the maximum bytes buffered for one uncompressed or compressed entry.
    auto setMaximumBufferedItemLength(unit::ByteLength value) noexcept -> ArchiveWriterOptions & {
        _maximumBufferedItemLength = value;
        return *this;
    }
    /// Get the maximum number of entries written into the archive.
    [[nodiscard]] auto maximumItemCount() const noexcept -> unit::ItemCount { return _maximumItemCount; }
    /// Set the maximum number of entries written into the archive.
    auto setMaximumItemCount(unit::ItemCount value) noexcept -> ArchiveWriterOptions & {
        _maximumItemCount = value;
        return *this;
    }

private:
    stream::InputStreamSettings _inputStreamSettings;   ///< Settings for owned streams.
    stream::OutputStreamSettings _outputStreamSettings; ///< Settings for owned streams.
    ZipProgressFn _progress;                            ///< Entry observer.

    Zip64Policy _zip64Policy{Zip64Policy::Automatic};
    unit::ByteLength _maximumBufferedItemLength{256U * 1024U * 1024U};
    unit::ItemCount _maximumItemCount{100000U};
};

}
