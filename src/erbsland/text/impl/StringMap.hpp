// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringHashSet.hpp"
#include "StringKey.hpp"
#include "StringList.hpp"
#include "StringMap_fwd.hpp"
#include "StringSet.hpp"

#include "../../util/Map.hpp"

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>

namespace erbsland::text::impl {

/// A string-keyed ordered map with Erbsland Core string-list helpers.
/// @tparam tString The string key type.
/// @tparam tValue The value type.
/// @tparam tCaseInsensitive Use case-insensitive key comparison.
/// @tested{StringMapTest}
template <typename tString, typename tValue, bool tCaseInsensitive>
class StringMap : public util::Map<
                      tString,
                      tValue,
                      std::conditional_t<tCaseInsensitive, StringCICompare<tString>, StringCompare<tString>>,
                      StringMap<tString, tValue, tCaseInsensitive>> {
public:
    using Compare = std::conditional_t<tCaseInsensitive, StringCICompare<tString>, StringCompare<tString>>;
    using Base = util::Map<tString, tValue, Compare, StringMap<tString, tValue, tCaseInsensitive>>;
    using Key = tString;
    using Entry = typename Base::Entry;
    using Raw = typename Base::Raw;

public:
    using Base::Base;
    using Base::contains;
    using Base::get;
    using Base::remove;
    using Base::removed;
    using Base::take;

    /// Creates a map from key-value entries.
    /// @param values The initial entries.
    StringMap(std::initializer_list<Entry> values) {
        for (const auto &[key, value] : values) {
            set(key, value);
        }
    }
    /// Creates a map by copying its raw representation.
    /// @param raw The raw map to copy.
    explicit StringMap(const Raw &raw) {
        for (const auto &[key, value] : raw) {
            set(key, value);
        }
    }
    /// Creates a map from its raw representation.
    /// @param raw The raw map to move values from.
    explicit StringMap(Raw &&raw) {
        for (auto &[key, value] : raw) {
            set(key, std::move(value));
        }
    }

    // defaults
    StringMap() = default;
    ~StringMap() = default;
    StringMap(const StringMap &) noexcept = default;
    StringMap(StringMap &&) noexcept = default;
    auto operator=(const StringMap &) noexcept -> StringMap & = default;
    auto operator=(StringMap &&) noexcept -> StringMap & = default;

public: // key access
    /// Get a value by a string key.
    [[nodiscard]] auto get(const Key &key) const -> std::optional<tValue> {
        const auto iterator = this->raw().find(key);
        if (iterator == this->raw().end()) {
            return {};
        }
        return iterator->second;
    }
    /// Get a value by a string key, with a default.
    [[nodiscard]] auto get(const Key &key, const tValue &defaultValue) const -> tValue {
        const auto iterator = this->raw().find(key);
        if (iterator == this->raw().end()) {
            return defaultValue;
        }
        return iterator->second;
    }
    /// Remove an entry by a string key.
    auto remove(const Key &key) -> StringMap & {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator != data.end()) {
            data.erase(iterator);
        }
        return *this;
    }
    /// Return a map with a string key removed.
    [[nodiscard]] auto removed(const Key &key) const -> StringMap {
        auto result = *this;
        result.remove(key);
        return result;
    }
    /// Take a value by a string key.
    auto take(const Key &key) -> tValue {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator == data.end()) {
            return {};
        }
        auto result = std::move(iterator->second);
        data.erase(iterator);
        return result;
    }
    /// Set a key-value pair from a string key.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    auto set(const Key &key, tValueFwd &&value) -> StringMap & {
        Base::set(key.copy(), std::forward<tValueFwd>(value));
        return *this;
    }
    /// Try to replace an existing value by a string key.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    [[nodiscard]] auto tryReplace(const Key &key, tValueFwd &&value) -> bool {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator == data.end()) {
            return false;
        }
        data.erase(iterator);
        data.emplace(key.copy(), std::forward<tValueFwd>(value));
        return true;
    }
    /// Try to insert a new entry from a string key.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    [[nodiscard]] auto tryInsert(const Key &key, tValueFwd &&value) -> bool {
        auto &data = this->mutableRaw();
        if (data.find(key) != data.end()) {
            return false;
        }
        data.emplace(key.copy(), std::forward<tValueFwd>(value));
        return true;
    }

public: // key tests
    /// Test if the map contains a string key.
    [[nodiscard]] auto contains(const Key &key) const -> bool { return this->raw().find(key) != this->raw().end(); }

public:
    /// Return the keys as a matching string list.
    [[nodiscard]] auto toKeyStringList() const -> StringList<tString> {
        return StringList<tString>{this->toKeyList().toStdVector()};
    }
    /// Return the keys as a matching string set.
    [[nodiscard]] auto toKeyStringSet() const -> StringSet<tString, tCaseInsensitive> {
        return StringSet<tString, tCaseInsensitive>::fromList(this->toKeyList());
    }
    /// Return the keys as a matching string hash set.
    [[nodiscard]] auto toKeyStringHashSet() const -> StringHashSet<tString, tCaseInsensitive> {
        return StringHashSet<tString, tCaseInsensitive>::fromList(this->toKeyList());
    }
};

}
