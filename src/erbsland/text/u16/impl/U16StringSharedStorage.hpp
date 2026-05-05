// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringData.hpp"
#include "U16StringDataView.hpp"
#include "U16StringLiteralStorage.hpp"

#include "../../../mem/StorageIdentifier.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../../unit/U16DataLength.hpp"
#include "../../../unit/U16DataRange.hpp"

#include <span>
#include <string_view>

namespace erbsland::text::impl {

/// Storage for shared UTF-16 string data.
/// @tested{U16StringTest}
class U16StringSharedStorage final {
public:
    /// Create empty shared storage.
    U16StringSharedStorage() = default;
    /// Create shared storage by copying a standard UTF-16 string view.
    explicit U16StringSharedStorage(std::u16string_view text);
    /// Create shared storage by copying a literal storage.
    explicit U16StringSharedStorage(const U16StringLiteralStorage &literal);
    /// Create shared storage by copying the selected data view.
    explicit U16StringSharedStorage(const U16StringDataView &view);
    /// Create shared storage from existing shared data and range.
    U16StringSharedStorage(U16StringDataPtr data, unit::U16DataRange range) noexcept;
    /// Create uninitialized storage for writing new data into it.
    explicit U16StringSharedStorage(unit::U16DataRange range) noexcept;

public: // defaults
    ~U16StringSharedStorage() = default;
    U16StringSharedStorage(const U16StringSharedStorage &) = default;
    U16StringSharedStorage(U16StringSharedStorage &&) = default;
    auto operator=(const U16StringSharedStorage &) -> U16StringSharedStorage & = default;
    auto operator=(U16StringSharedStorage &&) -> U16StringSharedStorage & = default;

public: // tests
    /// Test if the storage is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _range.isEmpty() || _data.isNull(); }

public: // accessors
    /// Access the shared data pointer.
    [[nodiscard]] auto sharedData() const noexcept -> const U16StringDataPtr & { return _data; }
    /// Access the storage range.
    [[nodiscard]] auto range() const noexcept -> unit::U16DataRange { return _range; }
    /// Access the string as a string pointer, or `nullptr` if empty.
    [[nodiscard]] auto data() const noexcept -> const char16_t * {
        if (_data.isNull()) {
            return nullptr;
        }
        return _data.constGet()->data();
    }
    /// Access the string as a string pointer, or `nullptr` if empty.
    [[nodiscard]] auto dataForWrite() noexcept -> char16_t * {
        if (_data.isNull()) {
            return nullptr;
        }
        return _data.get()->data();
    }
    /// Get the size of the string data.
    [[nodiscard]] auto dataSize() const noexcept -> std::size_t {
        if (_data.isNull()) {
            return 0;
        }
        return _data.get()->size() - 1U;
    }
    /// Get a unique identifier for the visible storage range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier {
        if (data() == nullptr) {
            return {};
        }
        const auto *begin = data() + _range.index().toSizeT();
        return mem::StorageIdentifier::fromMemoryRange(begin, begin + _range.length().toSizeT());
    }
    /// Test if the storage can be mutated in place without detaching or materializing a slice.
    [[nodiscard]] auto isUniqueFullRange() const noexcept -> bool {
        return !_data.isNull() && !_data.isShared() && isFullRange(_data.constGet()->size());
    }

public:
    /// Create string storage by copying the exact UTF-16 code units from a span.
    [[nodiscard]] static auto fromCodeUnits(std::span<const char16_t> codeUnits) -> U16StringSharedStorage;
    /// Create uninitialized string storage for the given UTF-16 code-unit size.
    [[nodiscard]] static auto forSize(std::size_t size) -> U16StringSharedStorage;
    /// Throw if a size cannot be represented by the UTF-16 data length type.
    static void validateSize(std::size_t size);
    /// Add two sizes and validate the result as a string UTF-16 data length.
    [[nodiscard]] static auto checkedAddSize(std::size_t first, std::size_t second, std::string_view reason)
        -> std::size_t;
    /// Multiply two sizes and validate the result as a string UTF-16 data length.
    [[nodiscard]] static auto checkedMultiplySize(std::size_t first, std::size_t second, std::string_view reason)
        -> std::size_t;

public:
    /// Clear the storage.
    void clear() noexcept;
    /// Ensure mutable full-range storage with at least the requested string capacity.
    void ensureMutableCapacity(std::size_t requiredCapacity);
    /// Resize the visible string data and write the trailing null byte.
    void resize(std::size_t size) noexcept;
    /// Reserve usable capacity for the current string value.
    void reserve(unit::U16DataLength capacity);
    /// Shrink storage to the exact size of the visible string value.
    void shrinkToFit();
    /// Get the usable capacity for the current string value.
    [[nodiscard]] auto capacity() const noexcept -> unit::U16DataLength;
    /// Get the estimated retained heap memory usage.
    [[nodiscard]] auto memoryUsage() const noexcept -> unit::ByteLength;
    /// Detach shared data and materialize sliced data when necessary.
    void detach();
    /// Get a view to this storage.
    [[nodiscard]] auto dataView() const noexcept -> U16StringDataView;
    /// Get a view to this storage with a custom range.
    [[nodiscard]] auto dataView(unit::U16DataRange range) const noexcept -> U16StringDataView;

private:
    /// Test if the range covers the complete string payload without the null terminator.
    [[nodiscard]] auto isFullRange(std::size_t dataSize) const noexcept -> bool;
    /// Materialize the visible string value into standalone full-range storage with the requested capacity.
    void rematerialize(std::size_t reservedCapacity);

private:
    U16StringDataPtr _data;                                 ///< The COW string data.
    unit::U16DataRange _range{unit::U16DataRange::empty()}; ///< The range of `_data` that this string represents.
};

}
