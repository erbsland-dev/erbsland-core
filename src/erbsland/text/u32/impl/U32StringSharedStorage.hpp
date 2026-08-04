// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringData_fwd.hpp"
#include "U32StringDataView.hpp"
#include "U32StringLiteralStorage.hpp"
#include "U32StringSharedStorage_fwd.hpp"

#include "../../../mem/StorageIdentifier.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/CpRange.hpp"

#include <span>
#include <string_view>

namespace erbsland::text::impl {

/// Storage for shared UTF-32 string data.
/// @tested{U32StringTest}
class U32StringSharedStorage final {
public:
    /// Create empty shared storage.
    constexpr U32StringSharedStorage() noexcept : _data{}, _range{unit::CpRange::empty()} {}
    /// Create shared storage by copying a standard UTF-32 string view.
    explicit U32StringSharedStorage(std::u32string_view text);
    /// Create shared storage by copying a literal storage.
    explicit U32StringSharedStorage(const U32StringLiteralStorage &literal);
    /// Create shared storage by copying the selected data view.
    explicit U32StringSharedStorage(const U32StringDataView &view);
    /// Create shared storage from existing shared data and range.
    U32StringSharedStorage(U32StringDataPtr data, unit::CpRange range) noexcept;
    /// Create uninitialized storage for writing new data into it.
    explicit U32StringSharedStorage(unit::CpRange range) noexcept;

public: // defaults
    ~U32StringSharedStorage();
    U32StringSharedStorage(const U32StringSharedStorage &);
    U32StringSharedStorage(U32StringSharedStorage &&) noexcept;
    /// Copy another UTF-32 shared storage object.
    auto operator=(const U32StringSharedStorage &) -> U32StringSharedStorage &;
    /// Move another UTF-32 shared storage object.
    auto operator=(U32StringSharedStorage &&) noexcept -> U32StringSharedStorage &;

public: // tests
    /// Test if the storage is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;

public: // accessors
    /// Access the shared data pointer.
    [[nodiscard]] auto sharedData() const noexcept -> const U32StringDataPtr & { return _data; }
    /// Access the storage range.
    [[nodiscard]] auto range() const noexcept -> unit::CpRange { return _range; }
    /// Access the string as a string pointer, or `nullptr` if empty.
    [[nodiscard]] auto data() const noexcept -> const char32_t *;
    /// Access the string as a string pointer, or `nullptr` if empty.
    [[nodiscard]] auto dataForWrite() noexcept -> char32_t *;
    /// Get the size of the string data.
    [[nodiscard]] auto dataSize() const noexcept -> std::size_t;
    /// Get a unique identifier for the visible storage range.
    [[nodiscard]] auto storageId() const noexcept -> mem::StorageIdentifier;
    /// Test if the storage can be mutated in place without detaching or materializing a slice.
    [[nodiscard]] auto isUniqueFullRange() const noexcept -> bool;

public:
    /// Create string storage by copying the exact UTF-32 code units from a span.
    [[nodiscard]] static auto fromCodeUnits(std::span<const char32_t> codeUnits) -> U32StringSharedStorage;
    /// Create uninitialized string storage for the given UTF-32 code-unit size.
    [[nodiscard]] static auto forSize(std::size_t size) -> U32StringSharedStorage;
    /// Throw if a size cannot be represented by the UTF-32 data length type.
    static void validateSize(std::size_t size);
    /// Add two sizes and validate the result as a string UTF-32 data length.
    [[nodiscard]] static auto checkedAddSize(std::size_t first, std::size_t second, std::string_view reason)
        -> std::size_t;
    /// Multiply two sizes and validate the result as a string UTF-32 data length.
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
    void reserve(unit::CpLength capacity);
    /// Shrink storage to the exact size of the visible string value.
    void shrinkToFit();
    /// Get the usable capacity for the current string value.
    [[nodiscard]] auto capacity() const noexcept -> unit::CpLength;
    /// Get the estimated retained heap memory usage.
    [[nodiscard]] auto memoryUsage() const noexcept -> unit::ByteLength;
    /// Detach shared data and materialize sliced data when necessary.
    void detach();
    /// Get a view to this storage.
    [[nodiscard]] auto dataView() const noexcept -> U32StringDataView;
    /// Get a view to this storage with a custom range.
    [[nodiscard]] auto dataView(unit::CpRange range) const noexcept -> U32StringDataView;

private:
    /// Test if the range covers the complete string payload without the null terminator.
    [[nodiscard]] auto isFullRange(std::size_t dataSize) const noexcept -> bool;
    /// Materialize the visible string value into standalone full-range storage with the requested capacity.
    void rematerialize(std::size_t reservedCapacity);

private:
    U32StringDataPtr _data;                       ///< The COW string data.
    unit::CpRange _range{unit::CpRange::empty()}; ///< The range of `_data` that this string represents.
};

}
