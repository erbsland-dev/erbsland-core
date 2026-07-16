// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SharedData.hpp"

#include "impl/SharedDataPointerTraits.hpp"

#include <cstdint>
#include <utility>

namespace erbsland::mem {

/// An intrusive copy-on-write pointer for shared data objects.
/// @seedoc{/reference/mem/cow_storage}
/// @warning This is an advanced data type, meant for people extending the library.
/// Do not use it unless you understand the implications and have a specific need.
///
/// @tparam tDataType A type supported by `impl::SharedDataPointerTraits`.
/// @tparam tManualDetach If `true`, mutable access does not detach automatically.
template <typename tDataType, bool tManualDetach = false>
    requires impl::SharedDataPointerTraits<tDataType>::isSupported
class SharedDataPointer final {
    using Traits = impl::SharedDataPointerTraits<tDataType>;

public:
    /// The managed shared data type.
    using Type = tDataType;
    /// The raw pointer type used for the managed data.
    using Pointer = tDataType *;

public: // Construction
    /// Create a null pointer that does not own any data.
    constexpr SharedDataPointer() noexcept : _data{nullptr} {};
    /// Create a copy and increase the reference count.
    /// @param copy The pointer to copy.
    SharedDataPointer(const SharedDataPointer &copy) noexcept : _data{copy._data} { addReference(_data); }
    /// Move, without changing the reference count.
    /// @param other The pointer to move from.
    SharedDataPointer(SharedDataPointer &&other) noexcept : _data{std::exchange(other._data, nullptr)} {}
    /// Initialize the pointer with a new data object.
    /// The pointer takes shared ownership by adding one reference. The object must not already be managed by another
    /// ownership mechanism.
    /// @param data A newly allocated data object, or `nullptr`.
    explicit SharedDataPointer(tDataType *data) noexcept : _data{data} { addReference(_data); }
    /// Destroy the pointer and release the shared data if no references remain.
    ~SharedDataPointer() { release(_data); }
    /// Assign another pointer to this one.
    /// @param other The pointer to copy.
    /// @return A reference to this pointer.
    auto operator=(const SharedDataPointer &other) noexcept -> SharedDataPointer & {
        if (this != &other) {
            reset(other._data);
        }
        return *this;
    }
    /// Move another pointer to this one.
    /// @param other The pointer to move from.
    /// @return A reference to this pointer.
    auto operator=(SharedDataPointer &&other) noexcept -> SharedDataPointer & {
        if (this != &other) {
            release(std::exchange(_data, std::exchange(other._data, nullptr)));
        }
        return *this;
    }
    /// Assign shared data.
    /// @param data A newly allocated data object, or `nullptr`.
    void operator=(tDataType *data) noexcept { reset(data); }

public: // Comparison
    /// Compare if two pointers reference the same data object.
    /// @param other The pointer to compare with.
    /// @return `true` if both pointers reference the same data object.
    auto operator==(const SharedDataPointer &other) const noexcept -> bool { return _data == other._data; }
    /// Compare if two pointers reference different data objects.
    /// @param other The pointer to compare with.
    /// @return `true` if the pointers reference different data objects.
    auto operator!=(const SharedDataPointer &other) const noexcept -> bool { return _data != other._data; }

public: // Actions
    /// Detach the data from the shared instance.
    /// If this pointer is null or already uniquely owns its data, this function does nothing.
    /// Otherwise, it clones the current data object, references the clone, and releases the old data.
    void detach() {
        if (_data != nullptr && referenceCounter(_data).isShared()) {
            detachImpl();
        }
    }

public: // Accessors
    /// Get the pointer to the data.
    /// Automatically detaches shared data unless manual detach mode is enabled.
    /// @return A mutable pointer to the managed data, or `nullptr`.
    auto get() -> tDataType * {
        if (!tManualDetach) {
            detach();
        }
        return _data;
    }
    /// Get a const pointer to the data.
    /// @return A const pointer to the managed data, or `nullptr`.
    auto get() const noexcept -> const tDataType * { return _data; }
    /// Get the pointer to the data, always const.
    /// @return A const pointer to the managed data, or `nullptr`.
    auto constGet() const noexcept -> const tDataType * { return _data; }
    /// Set the shared data to another instance.
    /// The new data receives one reference. The previously managed data is released and destroyed if this pointer held
    /// the last reference.
    /// @param data A newly allocated data object, or `nullptr`.
    void reset(tDataType *data = nullptr) noexcept {
        if (data != _data) {
            addReference(data);
            tDataType *oldData = std::exchange(_data, data);
            release(oldData);
        }
    }
    /// Swap two shared data pointers.
    /// @param other The pointer to swap with.
    void swap(SharedDataPointer &other) noexcept { std::swap(_data, other._data); }
    /// Swap two shared data pointers.
    /// @param a The first pointer.
    /// @param b The second pointer.
    friend void swap(SharedDataPointer &a, SharedDataPointer &b) noexcept { std::swap(a._data, b._data); }
    /// Check if this is a null pointer.
    /// @return `true` if this pointer does not manage data.
    [[nodiscard]] auto isNull() const noexcept -> bool { return _data == nullptr; }
    /// Check if the referenced data is shared with other pointers.
    /// @return `true` if the managed data has more than one reference.
    [[nodiscard]] auto isShared() const noexcept -> bool {
        return _data != nullptr && referenceCounter(_data).isShared();
    }
    /// Get the current reference count for the referenced data.
    /// @return The reference count, or zero for a null pointer.
    [[nodiscard]] auto useCount() const noexcept -> uint32_t {
        return _data != nullptr ? referenceCounter(_data).useCount() : 0;
    }
    /// Get a unique ID for the shared data.
    [[nodiscard]] auto storageId() const noexcept -> std::size_t {
        return static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(_data));
    }

public: // Operators
    /// Access the managed data as a mutable reference.
    /// Automatically detaches shared data unless manual detach mode is enabled.
    /// @return A mutable reference to the managed data.
    auto operator*() -> tDataType & {
        if constexpr (!tManualDetach) {
            detach();
        }
        return *_data;
    }
    /// Access the managed data as a const reference.
    /// @return A const reference to the managed data.
    auto operator*() const noexcept -> const tDataType & { return *_data; }
    /// Access the managed data as a mutable pointer.
    /// Automatically detaches shared data unless manual detach mode is enabled.
    /// @return A mutable pointer to the managed data.
    auto operator->() -> tDataType * {
        if constexpr (!tManualDetach) {
            detach();
        }
        return _data;
    }
    /// Access the managed data as a const pointer.
    /// @return A const pointer to the managed data.
    auto operator->() const noexcept -> const tDataType * { return _data; }
    /// Convert to a mutable raw pointer.
    /// Automatically detaches shared data unless manual detach mode is enabled.
    /// @return A mutable pointer to the managed data, or `nullptr`.
    explicit operator tDataType *() {
        if constexpr (!tManualDetach) {
            detach();
        }
        return _data;
    }
    /// Convert to a const raw pointer.
    /// @return A const pointer to the managed data, or `nullptr`.
    explicit operator const tDataType *() const noexcept { return _data; }

private:
    /// The implementation of the detach method.
    void detachImpl() {
        auto *copy = Traits::clone(_data);
        addReference(copy);
        auto *oldData = std::exchange(_data, copy);
        release(oldData);
    }
    static void addReference(tDataType *data) noexcept {
        if (data != nullptr) {
            referenceCounter(data).addReference();
        }
    }
    static void release(tDataType *data) noexcept {
        if (data != nullptr && referenceCounter(data).removeReference() == ReferenceCounter::NoReferences) {
            Traits::destroy(data);
        }
    }
    [[nodiscard]] static auto referenceCounter(tDataType *data) noexcept -> ReferenceCounter & {
        return Traits::referenceCounter(data);
    }
    [[nodiscard]] static auto referenceCounter(const tDataType *data) noexcept -> const ReferenceCounter & {
        return Traits::referenceCounter(data);
    }

private:
    tDataType *_data; ///< A pointer to the shared data.
};

}
