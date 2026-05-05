// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <utility>

namespace erbsland::mem {

/// Shared copy-on-write storage for regular C++ data types.
/// @seedoc{/reference/mem/cow_storage}
///
/// `CowStorage` always owns a valid data object. Copying the storage shares that object, while mutable access through
/// `data()` detaches first when the object is shared.
/// Methods that create, replace, or detach data may throw allocation errors or exceptions from `tDataType`.
///
/// @tparam tDataType The copy-constructible data type to store.
/// @tested{CowStorageTest}
template <std::copy_constructible tDataType>
class CowStorage final {
public:
    /// The stored data type.
    using Type = tDataType;

public:
    /// Create storage with a default-constructed data object.
    CowStorage()
        requires std::default_initializable<tDataType>
        : _data{std::make_shared<tDataType>()} {}
    /// Copy storage and share its data object.
    CowStorage(const CowStorage &) noexcept = default;
    /// Move storage while keeping the source object valid.
    CowStorage(CowStorage &&other) noexcept : _data{other._data} {}
    /// Destroy the storage.
    ~CowStorage() = default;
    /// Copy storage and share its data object.
    auto operator=(const CowStorage &) noexcept -> CowStorage & = default;
    /// Move storage while keeping the source object valid.
    auto operator=(CowStorage &&other) noexcept -> CowStorage & {
        if (this != &other) {
            _data = other._data;
        }
        return *this;
    }

public:
    /// Create storage from an existing data object.
    /// @param data The data object to move into the storage.
    /// @return New unique storage containing `data`.
    [[nodiscard]] static auto from(tDataType data) -> CowStorage {
        return CowStorage{std::make_shared<tDataType>(std::move(data))};
    }
    /// Create storage by constructing the data object in place.
    /// @param args The arguments forwarded to the data type constructor.
    /// @return New unique storage containing the constructed data object.
    template <typename... tArgs>
        requires std::constructible_from<tDataType, tArgs...>
    [[nodiscard]] static auto create(tArgs &&...args) -> CowStorage {
        return CowStorage{std::make_shared<tDataType>(std::forward<tArgs>(args)...)};
    }

public:
    /// Access the data for reading.
    /// @return A const reference to the stored data object.
    [[nodiscard]] auto data() const noexcept -> const tDataType & { return *_data; }
    /// Access the data for writing.
    /// If the data object is shared, this method creates a private copy first.
    /// @return A mutable reference to the stored data object.
    auto data() -> tDataType & {
        detach();
        return *_data;
    }
    /// Replace the stored data object with a new unique object.
    /// @param data The data object to move into the storage.
    void setData(tDataType data) { _data = std::make_shared<tDataType>(std::move(data)); }
    /// Replace the stored data object by constructing a new unique object in place.
    /// @param args The arguments forwarded to the data type constructor.
    template <typename... tArgs>
        requires std::constructible_from<tDataType, tArgs...>
    void emplaceData(tArgs &&...args) {
        _data = std::make_shared<tDataType>(std::forward<tArgs>(args)...);
    }

public:
    /// Ensure this storage uniquely owns its data object.
    void detach() {
        if (isShared()) {
            _data = std::make_shared<tDataType>(*_data);
        }
    }
    /// Test if this storage shares its data object with another storage.
    /// @return `true` if there is more than one owner for the data object.
    [[nodiscard]] auto isShared() const noexcept -> bool { return _data.use_count() > 1; }
    /// Get the current use count of the shared data object.
    /// @return The number of storages currently sharing the data object.
    [[nodiscard]] auto useCount() const noexcept -> long { return _data.use_count(); }
    /// Get a unique identifier for the currently stored data object.
    /// @return The address of the data object as an integer.
    [[nodiscard]] auto storageId() const noexcept -> std::size_t {
        return static_cast<std::size_t>(reinterpret_cast<std::uintptr_t>(_data.get()));
    }
    /// Swap two storage objects.
    /// @param other The storage to swap with.
    void swap(CowStorage &other) noexcept { _data.swap(other._data); }
    /// Swap two storage objects.
    /// @param a The first storage.
    /// @param b The second storage.
    friend void swap(CowStorage &a, CowStorage &b) noexcept { a.swap(b); }

private:
    explicit CowStorage(std::shared_ptr<tDataType> data) noexcept : _data{std::move(data)} {}

private:
    std::shared_ptr<tDataType> _data; ///< The shared data object.
};

}
