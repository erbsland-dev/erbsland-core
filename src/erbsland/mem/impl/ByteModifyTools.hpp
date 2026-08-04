// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BestGrowth.hpp"
#include "ByteBlockData.hpp"
#include "ByteDataView.hpp"
#include "ByteWriteTools.hpp"
#include "SecureErase.hpp"
#include "Throw.hpp"

#include "../ByteSpan.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <utility>

namespace erbsland::mem::impl {

using namespace text::literals;

/// Implements dynamic byte-storage mutations for shared and unique ownership.
/// Source views borrow their storage for the duration of each call. Potentially aliased raw spans are copied only
/// when detachment or reallocation could invalidate them.
/// @tested{ByteBlockTest ByteBufferTest}
template <typename tData = ByteBlockData, typename tDataOwner = ByteBlockDataPtr>
class ByteModifyTools final {
    using DataOwner = tDataOwner;
    using Data = tData;

public:
    /// Create mutation tools for an owning data pointer.
    explicit ByteModifyTools(DataOwner &data) noexcept : _data{data} {}

public: // accessors
    /// Get a view of the complete visible data.
    [[nodiscard]] auto dataView() const noexcept -> ByteDataView {
        if (isNull()) {
            return {};
        }
        const auto *data = constData();
        return ByteDataView{
            ConstByteSpan{data->data(), static_cast<std::size_t>(data->size())},
            unit::ByteRange::fromSizeT(static_cast<std::size_t>(data->size()))};
    }
    /// Get the visible byte length.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength {
        return isNull() ? unit::ByteLength::zero()
                        : unit::ByteLength::fromSizeT(static_cast<std::size_t>(constData()->size()));
    }
    /// Get the allocated byte capacity.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength {
        return isNull() ? unit::ByteLength::zero()
                        : unit::ByteLength::fromSizeT(static_cast<std::size_t>(constData()->capacity()));
    }
    /// Access writable visible bytes after detaching.
    [[nodiscard]] auto writableData() -> ByteSpan {
        ensureUnique();
        if (isNull()) {
            return {};
        }
        return {mutableData()->data(), static_cast<std::size_t>(mutableData()->size())};
    }

public: // storage
    /// Ensure uniquely owned storage.
    void detach() { ensureUnique(); }
    /// Release all storage.
    void reset() noexcept { _data.reset(); }
    /// Remove all visible bytes while preserving capacity.
    void clear() {
        if (isNull()) {
            return;
        }
        const auto oldCapacity = capacity().toSizeT();
        const auto sensitive = isSensitive();
        if (isShared()) {
            _data = createData(0U, oldCapacity, sensitive);
            return;
        }
        auto *data = mutableData();
        if constexpr (!cSupportsSharedOwnership) {
            if (sensitive) {
                impl::secureErase(std::as_writable_bytes(ByteSpan{data->data(), data->capacity()}));
            }
        }
        data->setSize(0U);
    }
    /// Reserve at least the requested capacity.
    void reserve(unit::ByteLength requestedCapacity) {
        if (requestedCapacity > capacity()) {
            ensureCapacity(requestedCapacity.toSizeTOrThrow());
        }
    }
    /// Reduce capacity to the visible length.
    void shrinkToFit() {
        const auto currentLength = length().toSizeT();
        if (currentLength == 0U) {
            _data.reset();
        } else if (isShared() || capacity().toSizeT() != currentLength) {
            _data = createData(dataView().dataSpan(), isSensitive());
        }
    }
    /// Securely erase complete capacity while preserving length and capacity.
    void secureErase() {
        if (capacity().isZero()) {
            return;
        }
        if (isShared()) {
            auto replacement = createData(length().toSizeT(), capacity().toSizeT(), isSensitive());
            auto *replacementData = mutableData(replacement);
            ByteWriteTools{ByteSpan{replacementData->data(), static_cast<std::size_t>(replacementData->capacity())}}
                .fill(unit::ByteRange::all(), Byte{});
            _data = std::move(replacement);
            return;
        }
        auto *data = mutableData();
        impl::secureErase(std::as_writable_bytes(ByteSpan{data->data(), data->capacity()}));
    }

public: // modification
    /// Resize visible storage, zero-filling growth.
    void resize(unit::ByteLength newLengthValue) {
        const auto oldLength = length().toSizeT();
        const auto newLength = newLengthValue.toSizeTOrThrow();
        if (newLength == oldLength) {
            return;
        }
        if (newLength < oldLength) {
            ensureUnique();
            auto *data = mutableData();
            if constexpr (!cSupportsSharedOwnership) {
                if (isSensitive()) {
                    impl::secureErase(
                        std::as_writable_bytes(ByteSpan{data->data() + newLength, oldLength - newLength}));
                }
            }
            data->setSize(static_cast<typename Data::SizeType>(newLength));
            return;
        }
        ensureCapacity(newLength);
        auto *data = mutableData();
        ByteWriteTools{ByteSpan{data->data(), newLength}}.fill(
            unit::ByteRange{unit::ByteIndex::fromSizeT(oldLength), unit::ByteLength::fromSizeT(newLength - oldLength)},
            Byte{});
        data->setSize(static_cast<typename Data::SizeType>(newLength));
    }
    /// Remove a clamped byte range.
    void remove(unit::ByteRange range) {
        const auto clamped = range.clampedTo(length());
        if (clamped.isEmpty()) {
            return;
        }
        ensureUnique();
        auto *data = mutableData();
        const auto oldLength = static_cast<std::size_t>(data->size());
        const auto start = clamped.index().toSizeT();
        const auto count = clamped.length().toSizeT();
        const auto tailStart = start + count;
        const auto tailLength = oldLength - tailStart;
        std::memmove(data->data() + start, data->data() + tailStart, tailLength * sizeof(Byte));
        const auto newLength = oldLength - count;
        if constexpr (!cSupportsSharedOwnership) {
            if (isSensitive()) {
                impl::secureErase(std::as_writable_bytes(ByteSpan{data->data() + newLength, count}));
            }
        }
        data->setSize(static_cast<typename Data::SizeType>(newLength));
    }
    /// Keep only a clamped byte range.
    void keep(unit::ByteRange range) {
        if (!range.isValid()) {
            clear();
            return;
        }
        const auto clamped = range.clampedTo(length());
        if (clamped.isEmpty()) {
            clear();
            return;
        }
        ensureUnique();
        auto *data = mutableData();
        const auto count = clamped.length().toSizeT();
        const auto oldLength = static_cast<std::size_t>(data->size());
        std::memmove(data->data(), data->data() + clamped.index().toSizeT(), count * sizeof(Byte));
        if constexpr (!cSupportsSharedOwnership) {
            if (isSensitive()) {
                impl::secureErase(std::as_writable_bytes(ByteSpan{data->data() + count, oldLength - count}));
            }
        }
        data->setSize(static_cast<typename Data::SizeType>(count));
    }
    /// Replace a clamped range with source bytes.
    void replace(unit::ByteRange range, ByteDataView sourceView, bool sourceIsSensitive) {
        if (!range.isValid()) {
            return;
        }
        const auto oldLength = length().toSizeT();
        const auto clamped = range.clampedTo(unit::ByteLength::fromSizeT(oldLength));
        const auto start = clamped.index().toSizeT();
        const auto removedLength = clamped.length().toSizeT();
        const auto sourceBytes = sourceView.dataSpan();
        if (sourceIsSensitive && !sourceBytes.empty()) {
            markAsSensitive();
        }
        if (!sourceBytes.empty() && sourceBytes.size() == removedLength &&
            sourceBytes.data() == dataView().dataSpan().data() + start) {
            return;
        }
        auto snapshot = DataOwner{};
        auto source = stableSource(sourceBytes, snapshot);
        const auto newLength = checkedCombinedLength(oldLength - removedLength, source.size());
        ensureCapacity(newLength);
        if (sourceIsSensitive && !source.empty()) {
            markAsSensitive();
        }
        auto *data = mutableData();
        const auto tailStart = start + removedLength;
        const auto tailLength = oldLength - tailStart;
        if (source.size() != removedLength && tailLength != 0U) {
            std::memmove(data->data() + start + source.size(), data->data() + tailStart, tailLength * sizeof(Byte));
        }
        if (!source.empty()) {
            std::memcpy(data->data() + start, source.data(), source.size() * sizeof(Byte));
        }
        if constexpr (!cSupportsSharedOwnership) {
            if (isSensitive() && newLength < oldLength) {
                impl::secureErase(std::as_writable_bytes(ByteSpan{data->data() + newLength, oldLength - newLength}));
            }
        }
        data->setSize(static_cast<typename Data::SizeType>(newLength));
    }
    /// Insert bytes at an index clamped to the end.
    void insert(const unit::ByteIndex index, const ByteDataView &source, const bool sourceIsSensitive) {
        if (!index.isValid()) {
            return;
        }
        const auto position = std::min(index.toSizeT(), length().toSizeT());
        replace(unit::ByteRange::emptyAt(unit::ByteIndex::fromSizeT(position)), source, sourceIsSensitive);
    }
    /// Append one or more bytes.
    void append(const Byte value, const unit::ByteLength length) {
        const auto oldLength = this->length().toSizeT();
        const auto newLength = checkedCombinedLength(oldLength, length.toSizeT());
        ensureCapacity(newLength);
        auto *data = mutableData();
        if (length.isOne()) {
            data->data()[oldLength] = value;
        } else {
            std::fill_n(data->data() + oldLength, length.toSizeT(), value);
        }
        data->setSize(static_cast<typename Data::SizeType>(newLength));
    }
    /// Append source bytes.
    void append(ByteDataView sourceView, bool sourceIsSensitive) {
        auto snapshot = DataOwner{};
        auto source = stableSource(sourceView.dataSpan(), snapshot);
        if (source.empty()) {
            return;
        }
        if (sourceIsSensitive) {
            markAsSensitive();
        }
        const auto oldLength = length().toSizeT();
        const auto newLength = checkedCombinedLength(oldLength, source.size());
        ensureCapacity(newLength);
        if (sourceIsSensitive) {
            markAsSensitive();
        }
        auto *data = mutableData();
        std::memcpy(data->data() + oldLength, source.data(), source.size() * sizeof(Byte));
        data->setSize(static_cast<typename Data::SizeType>(newLength));
    }
    /// Fill a clamped destination range.
    void fill(unit::ByteRange range, Byte value) {
        if (!range.isValid()) {
            return;
        }
        const auto clamped = range.clampedTo(length());
        if (clamped.isEmpty()) {
            return;
        }
        ByteWriteTools{writableData()}.fill(clamped, value);
    }
    /// Overwrite the largest possible part of a clamped destination range.
    void overwrite(unit::ByteRange range, ByteDataView sourceView, bool sourceIsSensitive) {
        const auto sourceBytes = sourceView.dataSpan();
        if (!range.isValid()) {
            return;
        }
        const auto clamped = range.clampedTo(length());
        if (clamped.isEmpty() || sourceBytes.empty()) {
            return;
        }
        if (sourceIsSensitive) {
            markAsSensitive();
        }
        ByteWriteTools{writableData()}.overwrite(clamped, ByteDataView{sourceBytes});
    }
    /// XOR visible bytes with an equal-length source.
    [[nodiscard]] auto xorWith(ByteDataView sourceView, bool sourceIsSensitive) -> bool {
        const auto sourceBytes = sourceView.dataSpan();
        if (sourceBytes.size() != length().toSizeT()) {
            return false;
        }
        if (sourceBytes.empty()) {
            return true;
        }
        if (sourceIsSensitive) {
            markAsSensitive();
        }
        ByteWriteTools{writableData()}.xorWith(unit::ByteRange::all(), ByteDataView{sourceBytes});
        return true;
    }
    /// XOR the largest possible part of a clamped destination range.
    void xorWith(unit::ByteRange range, ByteDataView sourceView, bool sourceIsSensitive) {
        const auto sourceBytes = sourceView.dataSpan();
        if (!range.isValid()) {
            return;
        }
        const auto clamped = range.clampedTo(length());
        if (clamped.isEmpty() || sourceBytes.empty()) {
            return;
        }
        if (sourceIsSensitive) {
            markAsSensitive();
        }
        ByteWriteTools{writableData()}.xorWith(clamped, ByteDataView{sourceBytes});
    }
    /// Append zero-filled bytes and return their first index.
    [[nodiscard]] auto appendZeroed(unit::ByteLength appendedLength) -> unit::ByteIndex {
        const auto offset = unit::ByteIndex::end(length());
        resize(length() + appendedLength);
        return offset;
    }

public: // factories
    /// Create storage by copying a byte span.
    [[nodiscard]] static auto createData(ConstByteSpan bytes, bool sensitive = false) -> DataOwner {
        auto result = createData(bytes.size(), bytes.size(), sensitive);
        if (!bytes.empty()) {
            std::memcpy(mutableData(result)->data(), bytes.data(), bytes.size() * sizeof(Byte));
        }
        return result;
    }
    /// Create storage with a visible size and capacity.
    /// @throws err::OutOfRangeError If `size` or `capacity` exceeds the supported storage limit.
    [[nodiscard]] static auto createData(std::size_t size, std::size_t capacity, bool sensitive = false) -> DataOwner {
        if constexpr (!std::same_as<DataOwner, Data>) {
            if (size == 0U && capacity == 0U) {
                return {};
            }
        }
        if (size > capacity || !Data::canAllocateWithCapacity(capacity)) {
            throw err::OutOfRangeError{"Byte storage size exceeds the supported limit"_el};
        }
        if constexpr (std::same_as<DataOwner, Data>) {
            return Data::create(
                static_cast<typename Data::SizeType>(size),
                static_cast<typename Data::SizeType>(capacity),
                sensitive ? Data::cSensitiveFlag : std::uint8_t{});
        } else {
            return DataOwner{Data::create(
                static_cast<typename Data::SizeType>(size),
                static_cast<typename Data::SizeType>(capacity),
                sensitive ? Data::cSensitiveFlag : std::uint8_t{})};
        }
    }

private:
    inline static constexpr bool cSupportsSharedOwnership = requires(const DataOwner &data) { data.isShared(); };

    /// Return a combined length or throw when it exceeds storage limits.
    [[nodiscard]] static auto checkedCombinedLength(std::size_t first, std::size_t second) -> std::size_t {
        constexpr auto cMaximumLength = static_cast<std::size_t>(std::numeric_limits<typename Data::SizeType>::max());
        if (math::willAddOverflow(first, second) || first + second > cMaximumLength) {
            throw err::OutOfRangeError{"Byte block size exceeds the supported limit"_el};
        }
        return first + second;
    }
    /// Detach shared storage before modifying it.
    void ensureUnique() {
        if (!isNull()) {
            if constexpr (requires { _data.detach(); }) {
                _data.detach();
            }
        }
    }
    /// Ensure that storage has the requested capacity.
    void ensureCapacity(std::size_t requestedCapacity) {
        const auto oldLength = length().toSizeT();
        if (!Data::canAllocateWithCapacity(requestedCapacity)) {
            throw err::OutOfRangeError{"Byte storage capacity exceeds the supported limit"_el};
        }
        const auto *oldData = constData();
        const auto oldCapacity = oldData == nullptr ? std::size_t{} : static_cast<std::size_t>(oldData->capacity());
        if (oldData != nullptr && !isShared() && requestedCapacity <= oldCapacity) {
            return;
        }
        const auto newCapacity = BestGrowth{oldCapacity, std::max(oldLength, requestedCapacity)}.bestGrowth<Data>();
        auto replacement = createData(oldLength, newCapacity, isSensitive());
        if (oldData != nullptr && oldLength != 0U) {
            std::memcpy(mutableData(replacement)->data(), oldData->data(), oldLength * sizeof(Byte));
        }
        _data = std::move(replacement);
    }
    /// Copy an overlapping source into stable temporary storage.
    [[nodiscard]] auto stableSource(ConstByteSpan source, DataOwner &snapshot) const -> ConstByteSpan {
        if (source.empty() || !overlapsStorage(source)) {
            return source;
        }
        snapshot = createData(source, isSensitive());
        const auto *data = constData(snapshot);
        return {data->data(), static_cast<std::size_t>(data->size())};
    }
    /// Test whether a source span overlaps the current storage.
    [[nodiscard]] auto overlapsStorage(ConstByteSpan source) const noexcept -> bool {
        if (isNull() || source.empty()) {
            return false;
        }
        const auto *data = constData();
        const auto storageBegin = reinterpret_cast<std::uintptr_t>(data->data());
        const auto storageEnd = storageBegin + static_cast<std::size_t>(data->capacity()) * sizeof(Byte);
        const auto sourceBegin = reinterpret_cast<std::uintptr_t>(source.data());
        const auto sourceEnd = sourceBegin + source.size() * sizeof(Byte);
        return sourceBegin < storageEnd && sourceEnd > storageBegin;
    }
    /// Test whether the current storage is sensitive.
    [[nodiscard]] auto isSensitive() const noexcept -> bool { return !isNull() && constData()->isSensitive(); }
    /// Mark the current storage as sensitive.
    void markAsSensitive() noexcept {
        if (!isNull()) {
            constData()->setSensitive();
        }
    }

    /// Test whether storage is null.
    [[nodiscard]] auto isNull() const noexcept -> bool {
        if constexpr (requires { _data.isNull(); }) {
            return _data.isNull();
        } else if constexpr (std::same_as<DataOwner, Data>) {
            return false;
        } else {
            return _data == nullptr;
        }
    }
    /// Test whether storage is shared.
    [[nodiscard]] auto isShared() const noexcept -> bool {
        if constexpr (requires { _data.isShared(); }) {
            return _data.isShared();
        } else {
            return false;
        }
    }
    /// Access the current immutable storage data.
    [[nodiscard]] auto constData() const noexcept -> const Data * { return constData(_data); }
    /// Access immutable data from a storage owner.
    [[nodiscard]] static auto constData(const DataOwner &data) noexcept -> const Data * {
        if constexpr (requires { data.constGet(); }) {
            return data.constGet();
        } else if constexpr (std::same_as<DataOwner, Data>) {
            return &data;
        } else {
            return data.get();
        }
    }
    /// Access the current mutable storage data.
    [[nodiscard]] auto mutableData() noexcept -> Data * { return mutableData(_data); }
    /// Access mutable data from a storage owner.
    [[nodiscard]] static auto mutableData(DataOwner &data) noexcept -> Data * {
        if constexpr (std::same_as<DataOwner, Data>) {
            return &data;
        } else {
            return data.get();
        }
    }

private:
    DataOwner &_data; ///< The owning storage mutated by these tools.
};

}
