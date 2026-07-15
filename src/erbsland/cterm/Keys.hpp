// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Key.hpp"

#include "../text/Char.hpp"
#include "../text/String.hpp"
#include "../text/StringList.hpp"

#include <cstddef>
#include <initializer_list>
#include <limits>
#include <vector>

namespace erbsland::cterm {

/// An ordered set of unique key presses for key bindings.
class Keys {
public:
    /// Storage type for the ordered key list.
    using Container = std::vector<Key>;
    /// Const iterator over keys.
    using const_iterator = Container::const_iterator;
    /// Number of leading keys shown in compact help.
    using MainCount = std::size_t;

public:
    /// Create an empty key set.
    Keys() = default;
    /// Create a key set with one key.
    /// @param key The key to add.
    Keys(Key key); // NOLINT(*-explicit-constructor)
    /// Create a key set with one special key.
    /// @param keyType The special key type to add.
    Keys(Key::Type keyType); // NOLINT(*-explicit-constructor)
    /// Create a key set with one character key.
    /// @param character The character key to add.
    Keys(text::Char character); // NOLINT(*-explicit-constructor)
    /// Create a key set from a list of keys.
    /// @param keys The keys to add in priority order.
    Keys(std::initializer_list<Key> keys); // NOLINT(*-explicit-constructor)
    /// Create a key set from a vector of keys.
    /// @param keys The keys to add in priority order.
    Keys(std::vector<Key> keys); // NOLINT(*-explicit-constructor)

public:                          // operators
    auto operator==(const Keys &other) const -> bool = default;
    auto operator!=(const Keys &other) const -> bool = default;

public: // accessors
    /// Test if this key set is empty.
    [[nodiscard]] auto empty() const noexcept -> bool { return _keys.empty(); }
    /// Get the number of keys.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return _keys.size(); }
    /// Get all keys in priority order.
    [[nodiscard]] auto keys() const noexcept -> const Container & { return _keys; }
    /// Get the number of main keys.
    [[nodiscard]] auto mainKeyCount() const noexcept -> std::size_t;
    /// Get the main keys shown in compact help.
    [[nodiscard]] auto mainKeys() const -> std::vector<Key>;
    /// Get alternative keys shown only in detailed help.
    [[nodiscard]] auto alternativeKeys() const -> std::vector<Key>;
    /// Get all key labels for the main keys.
    [[nodiscard]] auto mainKeyLabels() const -> text::StringList;

public: // modifiers
    /// Replace all keys.
    /// @param keys The keys to add in priority order.
    /// @return This object.
    auto setKeys(std::vector<Key> keys) -> Keys &;
    /// Replace all keys.
    /// @param keys The keys to add in priority order.
    /// @return This object.
    auto setKeys(std::initializer_list<Key> keys) -> Keys &;
    /// Add a key if it is not already present.
    /// @param key The key to add.
    /// @return This object.
    auto add(Key key) -> Keys &;
    /// Add a special key if it is not already present.
    /// @param keyType The special key type to add.
    /// @return This object.
    auto add(Key::Type keyType) -> Keys &;
    /// Add a character key if it is not already present.
    /// @param character The character key to add.
    /// @return This object.
    auto add(text::Char character) -> Keys &;
    /// Clear all keys and reset compact help to show all future keys.
    /// @return This object.
    auto clear() noexcept -> Keys &;
    /// Set how many leading keys are shown in compact help.
    /// @param mainKeyCount The number of leading keys considered main keys.
    /// @return This object.
    auto setMainKeyCount(MainCount mainKeyCount) noexcept -> Keys &;

public: // tests
    /// Test if the set contains a key.
    /// @param key The key to test.
    /// @return `true` if the key is part of the set.
    [[nodiscard]] auto contains(const Key &key) const noexcept -> bool;
    /// Test if the set matches a key event.
    /// @param key The key event to test.
    /// @return `true` if the key is part of the set.
    [[nodiscard]] auto matches(const Key &key) const noexcept { return contains(key); }

private:
    /// The sentinel value for "all keys are main keys".
    [[nodiscard]] static constexpr auto allKeysAreMain() noexcept -> MainCount {
        return std::numeric_limits<MainCount>::max();
    }
    /// Validate a key as a displayable key binding.
    /// @param key The key to validate.
    /// @throws err::ParameterError if the key is not displayable.
    static void validateKey(const Key &key);
    /// Validate and convert one special key type into a key.
    /// @param keyType The special key type.
    /// @return The converted key.
    [[nodiscard]] static auto keyFromType(Key::Type keyType) -> Key;

private:
    Container _keys;                           ///< The keys in priority order.
    MainCount _mainKeyCount{allKeysAreMain()}; ///< Count of leading keys shown in compact help.
};

}
