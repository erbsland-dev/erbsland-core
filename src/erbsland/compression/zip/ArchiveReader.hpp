// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ArchiveExtractionOptions.hpp"
#include "ArchiveItem.hpp"
#include "ArchiveReader_fwd.hpp"
#include "ArchiveReaderOptions.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../path/Path.hpp"
#include "../../stream/ByteInputStream_fwd.hpp"
#include "../../text/String.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../unit/ItemIndex.hpp"

#include <functional>

namespace erbsland::compression::zip {

/// A synchronous, inspection-and-extraction interface for one ZIP archive.
/// Parsing follows APPNOTE 6.3.10 sections 4.3.12 through 4.3.16 and 4.5.3.
/// @seedoc{/reference/compression/zip_archives}
/// Archive operations are unsuitable for sensitive data; codec storage is not securely erased.
/// @tested{ZipArchiveTest}
class ArchiveReader {
protected: // defaults
    ArchiveReader() = default;

public:
    using Filter = std::function<bool(const ArchiveItem &)>;

public: // defaults/deletions
    virtual ~ArchiveReader() = default;
    ArchiveReader(const ArchiveReader &) = delete;
    ArchiveReader(ArchiveReader &&) = delete;
    auto operator=(const ArchiveReader &) -> ArchiveReader & = delete;
    auto operator=(ArchiveReader &&) -> ArchiveReader & = delete;

public: // factories
    /// Open and own a path-backed ZIP archive.
    [[nodiscard]] static auto create(const path::Path &source, ArchiveReaderOptions options = {}) -> ArchiveReaderPtr;
    /// Open and own an in-memory ZIP archive.
    [[nodiscard]] static auto create(mem::ByteBlock source, ArchiveReaderOptions options = {}) -> ArchiveReaderPtr;
    /// Open and own a ZIP archive input stream.
    /// @throws err::ParameterError If `source` is null or does not support positioning.
    [[nodiscard]] static auto create(stream::ByteInputStreamPtr source, ArchiveReaderOptions options = {})
        -> ArchiveReaderPtr;

public: // directory
    /// Get the number of validated central-directory entries.
    [[nodiscard]] virtual auto itemCount() const noexcept -> unit::ItemCount = 0;
    /// Get one entry, or null when the index is outside the directory.
    [[nodiscard]] virtual auto item(unit::ItemIndex index) const noexcept -> ArchiveItemPtr = 0;
    /// Get a copy of the validated entries in central-directory order.
    [[nodiscard]] virtual auto items() const -> ArchiveItemList = 0;
    /// Get the archive comment decoded as tolerant UTF-8.
    [[nodiscard]] virtual auto comment() const noexcept -> const text::String & = 0;
    /// Test whether this archive uses ZIP64 records or fields.
    [[nodiscard]] virtual auto isZip64() const noexcept -> bool = 0;
    /// Extract every entry accepted by the optional filter below one output root.
    virtual void extractToDirectory(
        const path::Path &root, ArchiveExtractionOptions options = {}, Filter filter = {}) const = 0;

public: // lifecycle
    /// Close the owned input stream; repeated calls have no effect.
    virtual void close() noexcept = 0;
    /// Test whether payload operations can still access the owned input stream.
    [[nodiscard]] virtual auto isOpen() const noexcept -> bool = 0;
};

}
