// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringDataView.hpp"

#include "../../../mem/StorageIdentifier.hpp"
#include "../../../unit/CpRange.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text::impl {

/// Storage for UTF-32 string literal data.
/// @tested{U32StringLiteralTest}
class U32StringLiteralStorage final {
public:
    /// Create empty literal storage.
    constexpr U32StringLiteralStorage() noexcept = default;
    /// Create literal storage from a static character span.
    explicit constexpr U32StringLiteralStorage(const char32_t *data, const std::size_t size) noexcept :
        _data{data, size}, _range{unit::CpRange::fromSizeT(size)} {}
    /// Create literal storage from a static character span.
    explicit constexpr U32StringLiteralStorage(const std::span<const char32_t> data) noexcept :
        _data{data}, _range{unit::CpRange::fromSizeT(data.size())} {}
    /// Create literal storage from a static character span and range.
    constexpr U32StringLiteralStorage(const std::span<const char32_t> data, const unit::CpRange range) noexcept :
        _data{data}, _range{range} {}

public: // defaults
    ~U32StringLiteralStorage() = default;
    U32StringLiteralStorage(const U32StringLiteralStorage &) = default;
    U32StringLiteralStorage(U32StringLiteralStorage &&) = default;
    auto operator=(const U32StringLiteralStorage &) -> U32StringLiteralStorage & = default;
    auto operator=(U32StringLiteralStorage &&) -> U32StringLiteralStorage & = default;

public: // accessors
    /// Access the literal data.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char32_t> { return _data; }
    /// Access the storage range.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::CpRange { return _range; }
    /// Get the literal size in UTF-32 code units.
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
    [[nodiscard]] constexpr auto dataView() const noexcept -> U32StringDataView {
        return U32StringDataView{_data, _range};
    }
    /// Get a view to the literal data with a custom range.
    [[nodiscard]] constexpr auto dataView(const unit::CpRange range) const noexcept -> U32StringDataView {
        return U32StringDataView{_data, range};
    }

private:
    std::span<const char32_t> _data;              ///< Reference to the literal string data.
    unit::CpRange _range{unit::CpRange::empty()}; ///< The range of `_data` that this storage represents.
};

}
