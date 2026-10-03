// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringData_fwd.hpp"
#include "U16StringDataView.hpp"
#include "U16StringLiteralStorage.hpp"
#include "U16StringSharedStorage_fwd.hpp"

#include "../../../mem/StorageIdentifier.hpp"
#include "../../../unit/ByteCount.hpp"
#include "../../../unit/U16Count.hpp"
#include "../../../unit/U16Range.hpp"

#include <span>
#include <string_view>

namespace erbsland::text::impl {

/// Storage for shared UTF-16 string data.
/// @tested{U16StringTest StringStorageBoundsTest}
class U16StringSharedStorage final {
public:
    /// Create empty shared storage.
    constexpr U16StringSharedStorage() noexcept : _data{}, _range{unit::U16Range::empty()} {}
    /// Create shared storage by copying a standard UTF-16 string view.
    explicit U16StringSharedStorage(std::u16string_view text);
    /// Create shared storage by copying a literal storage.
    explicit U16StringSharedStorage(const U16StringLiteralStorage &literal);
    /// Create shared storage by copying the selected data view.
    explicit U16StringSharedStorage(const U16StringDataView &view);
    /// Create shared storage from existing shared data and range.
    U16StringSharedStorage(U16StringDataPtr data, unit::U16Range range) noexcept;
    /// Create uninitialized storage for writing new data into it.
    explicit U16StringSharedStorage(unit::U16Range range) noexcept;

public: // defaults
    ~U16StringSharedStorage();
    U16StringSharedStorage(const U16StringSharedStorage &);
    U16StringSharedStorage(U16StringSharedStorage &&) noexcept;
    auto operator=(const U16StringSharedStorage &) -> U16StringSharedStorage &;
    auto operator=(U16StringSharedStorage &&) noexcept -> U16StringSharedStorage &;

public: // tests
    /// Test if the storage is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;

public: // accessors
    /// Access the shared data pointer.
    [[nodiscard]] auto sharedData() const noexcept -> const U16StringDataPtr & { return _data; }
    /// Access the storage range.
    [[nodiscard]] auto range() const noexcept -> unit::U16Range { return _range; }
    /// Access the string as a string pointer, or `nullptr` if empty.
    [[nodiscard]] auto data() const noexcept -> const char16_t *;
    /// Access the string as a string pointer, or `nullptr` if empty.
    [[nodiscard]] auto dataForWrite() noexcept -> char16_t *;
    /// Get the size of the string data.
    [[nodiscard]] auto dataSize() const noexcept -> std::size_t;
    /// Get a unique identifier for the visible storage range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier;
    /// Test if the storage can be mutated in place without detaching or materializing a slice.
    [[nodiscard]] auto isUniqueFullRange() const noexcept -> bool;

public:
    /// Create string storage by copying the exact UTF-16 code units from a span.
    [[nodiscard]] static auto fromCodeUnits(std::span<const char16_t> codeUnits) -> U16StringSharedStorage;
    /// Create uninitialized string storage for the given UTF-16 code-unit size.
    [[nodiscard]] static auto forSize(std::size_t size) -> U16StringSharedStorage;
    /// Throw if a size cannot be represented by the UTF-16 data length type.
    /// @param size A finite size representable by the string length type.
    static void validateSize(std::size_t size);
    /// Add two sizes and validate the result as a string UTF-16 data length.
    /// @param first The first size.
    /// @param second The second size.
    /// @return The sum, which must fit the finite string length range.
    [[nodiscard]] static auto checkedAddSize(std::size_t first, std::size_t second) -> std::size_t;
    /// Multiply two sizes and validate the result as a string UTF-16 data length.
    /// @param first The first size.
    /// @param second The second size.
    /// @return The product, which must fit the finite string length range.
    [[nodiscard]] static auto checkedMultiplySize(std::size_t first, std::size_t second) -> std::size_t;

public:
    /// Clear the storage.
    void clear() noexcept;
    /// Ensure mutable full-range storage with at least the requested string capacity.
    void ensureMutableCapacity(std::size_t requiredCapacity);
    /// Resize the visible string data and write the trailing null byte.
    void resize(std::size_t size) noexcept;
    /// Reserve usable capacity for the current string value.
    void reserve(unit::U16Count capacity);
    /// Shrink storage to the exact size of the visible string value.
    void shrinkToFit();
    /// Get the usable capacity for the current string value.
    [[nodiscard]] auto capacity() const noexcept -> unit::U16Count;
    /// Get the estimated retained heap memory usage.
    [[nodiscard]] auto memoryUsage() const noexcept -> unit::ByteCount;
    /// Detach shared data and materialize sliced data when necessary.
    void detach();
    /// Get a view to this storage.
    [[nodiscard]] auto dataView() const noexcept -> U16StringDataView;
    /// Get a view to this storage with a custom range.
    [[nodiscard]] auto dataView(unit::U16Range range) const noexcept -> U16StringDataView;

private:
    /// Test if the range covers the complete string payload without the null terminator.
    [[nodiscard]] auto isFullRange(std::size_t dataSize) const noexcept -> bool;
    /// Materialize the visible string value into standalone full-range storage with the requested capacity.
    void rematerialize(std::size_t reservedCapacity);

private:
    U16StringDataPtr _data;                         ///< The COW string data.
    unit::U16Range _range{unit::U16Range::empty()}; ///< The range of `_data` that this string represents.
};

}
