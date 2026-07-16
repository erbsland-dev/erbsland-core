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

/// A string-keyed ordered map with Erbsland string-list helpers.
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
    using View = typename StringViewFor<tString>::Type;

public:
    using Base::Base;
    using Base::contains;
    using Base::get;
    using Base::remove;
    using Base::removed;
    using Base::set;
    using Base::take;
    using Base::tryInsert;
    using Base::tryReplace;
    StringMap() = default;
    ~StringMap() = default;
    StringMap(const StringMap &) noexcept = default;
    StringMap(StringMap &&) noexcept = default;
    auto operator=(const StringMap &) noexcept -> StringMap & = default;
    auto operator=(StringMap &&) noexcept -> StringMap & = default;

public: // view key access
    /// Get a value by a string view key.
    [[nodiscard]] auto get(const View &key) const -> std::optional<tValue> {
        const auto iterator = this->raw().find(key);
        if (iterator == this->raw().end()) {
            return {};
        }
        return iterator->second;
    }
    /// Get a value by a string view key, with a default.
    [[nodiscard]] auto get(const View &key, const tValue &defaultValue) const -> tValue {
        const auto iterator = this->raw().find(key);
        if (iterator == this->raw().end()) {
            return defaultValue;
        }
        return iterator->second;
    }
    /// Remove an entry by a string view key.
    auto remove(const View &key) -> StringMap & {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator != data.end()) {
            data.erase(iterator);
        }
        return *this;
    }
    /// Return a map with a string view key removed.
    [[nodiscard]] auto removed(const View &key) const -> StringMap {
        auto result = *this;
        result.remove(key);
        return result;
    }
    /// Take a value by a string view key.
    auto take(const View &key) -> tValue {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator == data.end()) {
            return {};
        }
        auto result = std::move(iterator->second);
        data.erase(iterator);
        return result;
    }
    /// Set a key-value pair from a string view key.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    auto set(const View &key, tValueFwd &&value) -> StringMap & {
        Base::set(tString{key}, std::forward<tValueFwd>(value));
        return *this;
    }
    /// Try to replace an existing value by a string view key.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    [[nodiscard]] auto tryReplace(const View &key, tValueFwd &&value) -> bool {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator == data.end()) {
            return false;
        }
        data.erase(iterator);
        data.emplace(tString{key}, std::forward<tValueFwd>(value));
        return true;
    }
    /// Try to insert a new entry from a string view key.
    template <typename tValueFwd>
        requires std::constructible_from<tValue, tValueFwd &&>
    [[nodiscard]] auto tryInsert(const View &key, tValueFwd &&value) -> bool {
        auto &data = this->mutableRaw();
        if (data.find(key) != data.end()) {
            return false;
        }
        data.emplace(tString{key}, std::forward<tValueFwd>(value));
        return true;
    }

public: // view key tests
    /// Test if the map contains a string view key.
    [[nodiscard]] auto contains(const View &key) const -> bool { return this->raw().find(key) != this->raw().end(); }

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
