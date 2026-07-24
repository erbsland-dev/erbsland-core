// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Throw.hpp"
#include "UnsafeByteBlockAccess_fwd.hpp"

#include "../ByteBlock.hpp"
#include "../ByteBlockEditor.hpp"
#include "../ByteSpan.hpp"

#include "../../unit/ByteIndex.hpp"

namespace erbsland::mem::impl {

/// Gives low-level implementations explicit borrowed access to byte-block storage.
/// Mutable access detaches an editor. Returned spans become invalid when the source is modified or destroyed.
/// @warning Never retain or expose the returned spans beyond the immediate low-level operation.
/// @tested{ByteBlockTest}
class UnsafeByteBlockAccess final {
public:
    explicit UnsafeByteBlockAccess(const ByteBlock &block) noexcept;
    explicit UnsafeByteBlockAccess(const ByteBlockEditor &block) noexcept;
    explicit UnsafeByteBlockAccess(ByteBlockEditor &block) noexcept;

    // defaults/deletions
    UnsafeByteBlockAccess() = delete;
    ~UnsafeByteBlockAccess() = default;
    UnsafeByteBlockAccess(const UnsafeByteBlockAccess &) = default;
    UnsafeByteBlockAccess(UnsafeByteBlockAccess &&) noexcept = default;
    auto operator=(const UnsafeByteBlockAccess &) -> UnsafeByteBlockAccess & = delete;
    auto operator=(UnsafeByteBlockAccess &&) -> UnsafeByteBlockAccess & = delete;

public:
    /// Access the visible bytes without detaching.
    [[nodiscard]] auto data() const noexcept -> ConstByteSpan;
    /// Access an exact-size read-only range.
    template <std::size_t Extent>
    [[nodiscard]] auto data(unit::ByteIndex offset = unit::ByteIndex::zero()) const -> FixedConstByteSpan<Extent> {
        const auto bytes = data();
        if (!offset.isValid() || offset.toSizeT() > bytes.size() || Extent > bytes.size() - offset.toSizeT()) {
            throwOutOfRange("Fixed byte-block range out of range");
        }
        return FixedConstByteSpan<Extent>{bytes.subspan(offset.toSizeT(), Extent)};
    }
    /// Access the visible writable bytes after detaching.
    [[nodiscard]] auto writableData() -> ByteSpan;
    /// Access an exact-size writable range after validation and detachment.
    template <std::size_t Extent>
    [[nodiscard]] auto writableData(unit::ByteIndex offset = unit::ByteIndex::zero()) -> FixedByteSpan<Extent> {
        const auto bytes = data();
        if (!offset.isValid() || offset.toSizeT() > bytes.size() || Extent > bytes.size() - offset.toSizeT()) {
            throwOutOfRange("Fixed byte-block range out of range");
        }
        return FixedByteSpan<Extent>{writableData().subspan(offset.toSizeT(), Extent)};
    }

private:
    const ByteBlock *_block{};
    ByteBlockEditor *_editor{};
    const ByteBlockEditor *_reader{};
};

}
