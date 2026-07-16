// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringData_fwd.hpp"
#include "U8StringDataView.hpp"
#include "U8StringLiteralStorage.hpp"

#include "../../../mem/StorageIdentifier.hpp"
#include "../../../mem/UnsafeCharPtr.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../../unit/ByteRange.hpp"

#include <span>
#include <string_view>

namespace erbsland::text::impl {

/// Storage for shared UTF-8 string data.
/// @tested{U8StringSharedStorageTest}
class U8StringSharedStorage final {
public:
    /// Create empty shared storage.
    constexpr U8StringSharedStorage() noexcept : _data{}, _range{unit::ByteRange::empty()} {}
    /// Create shared storage by copying a standard string view.
    explicit U8StringSharedStorage(std::string_view text);
    /// Create shared storage by copying a standard UTF-8 string view.
    explicit U8StringSharedStorage(std::u8string_view text);
    /// Create shared storage by copying a literal storage.
    explicit U8StringSharedStorage(const U8StringLiteralStorage &literal);
    /// Create shared storage by copying the selected data view.
    explicit U8StringSharedStorage(const U8StringDataView &view);
    /// Create shared storage from existing shared data and range.
    U8StringSharedStorage(U8StringDataPtr data, unit::ByteRange range) noexcept;
    /// Create uninitialized storage for writing new data into it.
    explicit U8StringSharedStorage(unit::ByteRange range) noexcept;

public: // defaults
    ~U8StringSharedStorage();
    U8StringSharedStorage(const U8StringSharedStorage &);
    U8StringSharedStorage(U8StringSharedStorage &&) noexcept;
    auto operator=(const U8StringSharedStorage &) -> U8StringSharedStorage &;
    auto operator=(U8StringSharedStorage &&) noexcept -> U8StringSharedStorage &;

public: // tests
    /// Test if the storage is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;

public: // accessors
    /// Access the shared data pointer.
    [[nodiscard]] auto sharedData() const noexcept -> const U8StringDataPtr & { return _data; }
    /// Access the storage range.
    [[nodiscard]] auto range() const noexcept -> unit::ByteRange { return _range; }
    /// Access the string as a string pointer, or `nullptr` if empty.
    [[nodiscard]] auto data() const noexcept -> mem::UnsafeConstCharPtr;
    /// Access the string as a string pointer, or `nullptr` if empty.
    [[nodiscard]] auto dataForWrite() noexcept -> mem::UnsafeCharPtr;
    /// Get the size of the string data.
    [[nodiscard]] auto dataSize() const noexcept -> std::size_t;
    /// Get a unique identifier for the visible storage range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier;
    /// Test if the storage can be mutated in place without detaching or materializing a slice.
    [[nodiscard]] auto isUniqueFullRange() const noexcept -> bool;

public:
    /// Create string storage by copying the exact bytes from a span.
    [[nodiscard]] static auto fromBytes(std::span<const char> bytes) -> U8StringSharedStorage;
    /// Create uninitialized string storage for the given byte size.
    [[nodiscard]] static auto forSize(std::size_t size) -> U8StringSharedStorage;
    /// Throw if a size cannot be represented by the byte length type.
    static void validateSize(std::size_t size);
    /// Add two sizes and validate the result as a string byte length.
    [[nodiscard]] static auto checkedAddSize(std::size_t first, std::size_t second, std::string_view reason)
        -> std::size_t;
    /// Multiply two sizes and validate the result as a string byte length.
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
    void reserve(unit::ByteLength capacity);
    /// Shrink storage to the exact size of the visible string value.
    void shrinkToFit();
    /// Get the usable capacity for the current string value.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength;
    /// Get the estimated retained heap memory usage.
    [[nodiscard]] auto memoryUsage() const noexcept -> unit::ByteLength;
    /// Detach shared data and materialize sliced data when necessary.
    void detach();
    /// Get a view to this storage.
    [[nodiscard]] auto dataView() const noexcept -> U8StringDataView;
    /// Get a view to this storage with a custom range.
    [[nodiscard]] auto dataView(unit::ByteRange range) const noexcept -> U8StringDataView;

private:
    /// Test if the range covers the complete string payload without the null terminator.
    [[nodiscard]] auto isFullRange(std::size_t dataSize) const noexcept -> bool;
    /// Materialize the visible string value into standalone full-range storage with the requested capacity.
    void rematerialize(std::size_t reservedCapacity);

private:
    U8StringDataPtr _data;                            ///< The COW string data.
    unit::ByteRange _range{unit::ByteRange::empty()}; ///< The range of `_data` that this string represents.
};

}
