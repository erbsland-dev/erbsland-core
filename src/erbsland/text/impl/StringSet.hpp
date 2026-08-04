// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringKey.hpp"
#include "StringList.hpp"
#include "StringSet_fwd.hpp"

#include "../../util/Set.hpp"

#include <compare>
#include <type_traits>

namespace erbsland::text::impl {

/// A string-keyed ordered set with Erbsland string-list helpers.
/// @tparam tString The string key type.
/// @tparam tCaseInsensitive Use case-insensitive key comparison.
/// @tested{StringSetTest}
template <typename tString, bool tCaseInsensitive>
class StringSet : public util::Set<
                      tString,
                      std::conditional_t<tCaseInsensitive, StringCICompare<tString>, StringCompare<tString>>,
                      StringSet<tString, tCaseInsensitive>> {
public:
    using Compare = std::conditional_t<tCaseInsensitive, StringCICompare<tString>, StringCompare<tString>>;
    using Base = util::Set<tString, Compare, StringSet<tString, tCaseInsensitive>>;
    using Key = tString;
    using Raw = typename Base::Raw;

public:
    using Base::Base;
    using Base::contains;
    using Base::remove;
    using Base::removed;
    using Base::tryRemove;

    /// Creates a set from initial keys.
    /// @param values The initial keys.
    StringSet(std::initializer_list<Key> values) {
        for (const auto &key : values) {
            insert(key);
        }
    }
    /// Creates a set by copying its raw representation.
    /// @param raw The raw set to copy.
    explicit StringSet(const Raw &raw) {
        for (const auto &key : raw) {
            insert(key);
        }
    }
    /// Creates a set from its raw representation.
    /// @param raw The raw set to move keys from.
    explicit StringSet(Raw &&raw) {
        for (const auto &key : raw) {
            insert(key);
        }
    }

    // defaults
    StringSet() = default;
    ~StringSet() = default;
    StringSet(const StringSet &) noexcept = default;
    StringSet(StringSet &&) noexcept = default;
    auto operator=(const StringSet &) noexcept -> StringSet & = default;
    auto operator=(StringSet &&) noexcept -> StringSet & = default;

public: // key changes
    /// Create a set from a list while compacting all keys.
    [[nodiscard]] static auto fromList(const util::List<Key> &values) -> StringSet {
        auto result = StringSet{};
        for (const auto &key : values) {
            result.insert(key);
        }
        return result;
    }
    /// Remove a string key.
    auto remove(const Key &key) -> StringSet & {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator != data.end()) {
            data.erase(iterator);
        }
        return *this;
    }
    /// Try to remove a string key.
    [[nodiscard]] auto tryRemove(const Key &key) -> bool {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator == data.end()) {
            return false;
        }
        data.erase(iterator);
        return true;
    }
    /// Return a set with a string key removed.
    [[nodiscard]] auto removed(const Key &key) const -> StringSet {
        auto result = *this;
        result.remove(key);
        return result;
    }
    /// Insert a string key.
    auto insert(const Key &key) -> StringSet & {
        auto &data = this->mutableRaw();
        if (data.find(key) == data.end()) {
            data.insert(key.copy());
        }
        return *this;
    }
    /// Try to insert a string key.
    [[nodiscard]] auto tryInsert(const Key &key) -> bool {
        auto &data = this->mutableRaw();
        if (data.find(key) != data.end()) {
            return false;
        }
        data.insert(key.copy());
        return true;
    }

public: // key tests
    /// Test if the set contains a string key.
    [[nodiscard]] auto contains(const Key &key) const -> bool { return this->raw().find(key) != this->raw().end(); }

public:
    /// Compare this set with another set using Unicode simple case folding.
    template <bool tOtherCaseInsensitive>
    [[nodiscard]] auto compareCI(const StringSet<tString, tOtherCaseInsensitive> &other) const -> bool {
        if (this->count() != other.count()) {
            return false;
        }
        return this->allOf([&](const tString &key) -> bool {
            return other.anyOf([&](const tString &otherKey) -> bool {
                return key.compare(otherKey, Char::compareCaseFolded) == std::strong_ordering::equal;
            });
        });
    }

    /// Return the keys as a matching string list.
    [[nodiscard]] auto toStringList() const -> StringList<tString> { return StringList<tString>{this->toStdVector()}; }
};

}
