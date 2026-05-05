// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringDataView.hpp"

#include "../../../mem/StorageIdentifier.hpp"
#include "../../../mem/UnsafeCharPtr.hpp"
#include "../../../unit/ByteRange.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text::impl {

/// Storage for UTF-8 string literal data.
/// @tested{U8StringLiteralStorageTest}
class U8StringLiteralStorage final {
public:
    /// Create empty literal storage.
    constexpr U8StringLiteralStorage() noexcept = default;
    /// Create literal storage from a static character span.
    explicit constexpr U8StringLiteralStorage(const mem::UnsafeConstCharPtr data, const std::size_t size) noexcept :
        _data{data, size}, _range{unit::ByteRange::fromSizeT(size)} {}
    /// Create literal storage from a static character span.
    explicit U8StringLiteralStorage(const mem::UnsafeConstChar8Ptr data, const std::size_t size) noexcept :
        _data{reinterpret_cast<const char *>(data), size}, _range{unit::ByteRange::fromSizeT(size)} {}
    /// Create literal storage from a static character span.
    explicit constexpr U8StringLiteralStorage(const std::span<const char> data) noexcept :
        _data{data}, _range{unit::ByteRange::fromSizeT(data.size())} {}
    /// Create literal storage from a static character span and range.
    constexpr U8StringLiteralStorage(const std::span<const char> data, const unit::ByteRange range) noexcept :
        _data{data}, _range{range} {}

public: // defaults
    ~U8StringLiteralStorage() = default;
    U8StringLiteralStorage(const U8StringLiteralStorage &) = default;
    U8StringLiteralStorage(U8StringLiteralStorage &&) = default;
    auto operator=(const U8StringLiteralStorage &) -> U8StringLiteralStorage & = default;
    auto operator=(U8StringLiteralStorage &&) -> U8StringLiteralStorage & = default;

public: // accessors
    /// Access the literal data.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char> { return _data; }
    /// Access the storage range.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::ByteRange { return _range; }
    /// Get the literal size in bytes.
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { return _data.size(); }
    /// Get a unique identifier for the visible storage range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier {
        if (_data.data() == nullptr) {
            return {};
        }
        const auto *begin = _data.data() + _range.index().toSizeT();
        return mem::StorageIdentifier::fromMemoryRange(begin, begin + _range.length().toSizeT());
    }

public:
    /// Get a view to the whole literal data.
    [[nodiscard]] constexpr auto dataView() const noexcept -> U8StringDataView {
        return U8StringDataView{_data, _range};
    }
    /// Get a view to the literal data with a custom range.
    [[nodiscard]] constexpr auto dataView(const unit::ByteRange range) const noexcept -> U8StringDataView {
        return U8StringDataView{_data, range};
    }

private:
    std::span<const char> _data;                      ///< Reference to the literal string data.
    unit::ByteRange _range{unit::ByteRange::empty()}; ///< The range of `_data` that this storage represents.
};

}
