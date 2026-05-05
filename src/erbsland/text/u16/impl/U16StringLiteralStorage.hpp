// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringDataView.hpp"

#include "../../../mem/StorageIdentifier.hpp"
#include "../../../unit/U16DataRange.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text::impl {

/// Storage for UTF-16 string literal data.
/// @tested{U16StringLiteralTest}
class U16StringLiteralStorage final {
public:
    /// Create empty literal storage.
    constexpr U16StringLiteralStorage() noexcept = default;
    /// Create literal storage from a static character span.
    explicit constexpr U16StringLiteralStorage(const char16_t *data, const std::size_t size) noexcept :
        _data{data, size}, _range{unit::U16DataRange::fromSizeT(size)} {}
    /// Create literal storage from a static character span.
    explicit constexpr U16StringLiteralStorage(const std::span<const char16_t> data) noexcept :
        _data{data}, _range{unit::U16DataRange::fromSizeT(data.size())} {}
    /// Create literal storage from a static character span and range.
    constexpr U16StringLiteralStorage(const std::span<const char16_t> data, const unit::U16DataRange range) noexcept :
        _data{data}, _range{range} {}

public: // defaults
    ~U16StringLiteralStorage() = default;
    U16StringLiteralStorage(const U16StringLiteralStorage &) = default;
    U16StringLiteralStorage(U16StringLiteralStorage &&) = default;
    auto operator=(const U16StringLiteralStorage &) -> U16StringLiteralStorage & = default;
    auto operator=(U16StringLiteralStorage &&) -> U16StringLiteralStorage & = default;

public: // accessors
    /// Access the literal data.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char16_t> { return _data; }
    /// Access the storage range.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::U16DataRange { return _range; }
    /// Get the literal size in UTF-16 code units.
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
    [[nodiscard]] constexpr auto dataView() const noexcept -> U16StringDataView {
        return U16StringDataView{_data, _range};
    }
    /// Get a view to the literal data with a custom range.
    [[nodiscard]] constexpr auto dataView(const unit::U16DataRange range) const noexcept -> U16StringDataView {
        return U16StringDataView{_data, range};
    }

private:
    std::span<const char16_t> _data;                        ///< Reference to the literal string data.
    unit::U16DataRange _range{unit::U16DataRange::empty()}; ///< The range of `_data` that this storage represents.
};

}
