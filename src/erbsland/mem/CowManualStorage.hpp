// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CowStorage.hpp"

#include <concepts>
#include <utility>

namespace erbsland::mem {

/// Shared copy-on-write storage with explicit writable access.
/// @seedoc{/reference/mem/memory_and_byte_data}
///
/// `CowManualStorage` always owns a valid data object. Copying the storage shares that object. Reading uses `data()`;
/// writing uses `detachedData()`, which detaches before returning a mutable reference.
/// Methods that create, replace, or detach data may throw allocation errors or exceptions from `tDataType`.
///
/// @tparam tDataType The copy-constructible data type to store.
/// @tested{CowStorageTest}
template <std::copy_constructible tDataType>
class CowManualStorage final {
public:
    /// The stored data type.
    using Type = tDataType;

public:
    /// Create storage with a default-constructed data object.
    CowManualStorage()
        requires std::default_initializable<tDataType>
        : _storage{} {}
    // defaults
    CowManualStorage(const CowManualStorage &) noexcept = default;
    CowManualStorage(CowManualStorage &&) noexcept = default;
    ~CowManualStorage() = default;
    auto operator=(const CowManualStorage &) noexcept -> CowManualStorage & = default;
    auto operator=(CowManualStorage &&) noexcept -> CowManualStorage & = default;

public:
    /// Create storage from an existing data object.
    /// @param data The data object to move into the storage.
    /// @return New unique storage containing `data`.
    [[nodiscard]] static auto from(tDataType data) -> CowManualStorage {
        return CowManualStorage{CowStorage<tDataType>::from(std::move(data))};
    }
    /// Create storage by constructing the data object in place.
    /// @param args The arguments forwarded to the data type constructor.
    /// @return New unique storage containing the constructed data object.
    template <typename... tArgs>
        requires std::constructible_from<tDataType, tArgs...>
    [[nodiscard]] static auto create(tArgs &&...args) -> CowManualStorage {
        return CowManualStorage{CowStorage<tDataType>::create(std::forward<tArgs>(args)...)};
    }
    /// Create storage that shares one default-constructed data object.
    /// The shared default object is retained for the lifetime of the program and contributes one owner to
    /// `useCount()`. Mutable access detaches before returning the data object.
    /// @return Storage sharing the default data object for this specialization.
    [[nodiscard]] static auto sharedDefault() -> CowManualStorage
        requires std::default_initializable<tDataType>
    {
        static const auto storage = CowManualStorage{};
        return storage;
    }

public:
    /// Access the data for reading.
    /// @return A const reference to the stored data object.
    [[nodiscard]] auto data() const noexcept -> const tDataType & { return _storage.data(); }
    /// Access the data for writing.
    /// If the data object is shared, this method creates a private copy first.
    /// @return A mutable reference to the stored data object.
    auto detachedData() -> tDataType & { return _storage.data(); }
    /// Replace the stored data object with a new unique object.
    /// @param data The data object to move into the storage.
    void setData(tDataType data) { _storage.setData(std::move(data)); }
    /// Replace the stored data object by constructing a new unique object in place.
    /// @param args The arguments forwarded to the data type constructor.
    template <typename... tArgs>
        requires std::constructible_from<tDataType, tArgs...>
    void emplaceData(tArgs &&...args) {
        _storage.emplaceData(std::forward<tArgs>(args)...);
    }

public:
    /// Ensure this storage uniquely owns its data object.
    void detach() { _storage.detach(); }
    /// Test if this storage shares its data object with another storage.
    /// @return `true` if there is more than one owner for the data object.
    [[nodiscard]] auto isShared() const noexcept -> bool { return _storage.isShared(); }
    /// Get the current use count of the shared data object.
    /// @return The number of storages currently sharing the data object.
    [[nodiscard]] auto useCount() const noexcept -> long { return _storage.useCount(); }
    /// Get a unique identifier for the currently stored data object.
    /// @return The address of the data object as an integer.
    [[nodiscard]] auto storageId() const noexcept -> std::size_t { return _storage.storageId(); }
    /// Swap two storage objects.
    /// @param other The storage to swap with.
    void swap(CowManualStorage &other) noexcept { _storage.swap(other._storage); }
    /// Swap two storage objects.
    /// @param a The first storage.
    /// @param b The second storage.
    friend void swap(CowManualStorage &a, CowManualStorage &b) noexcept { a.swap(b); }

private:
    /// Create manual storage from its shared implementation.
    explicit CowManualStorage(CowStorage<tDataType> storage) noexcept : _storage{std::move(storage)} {}

private:
    CowStorage<tDataType> _storage; ///< The shared storage implementation.
};

}
