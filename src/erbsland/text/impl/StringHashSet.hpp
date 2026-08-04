// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringHashSet_fwd.hpp"
#include "StringKey.hpp"
#include "StringList.hpp"

#include "../../util/HashSet.hpp"

#include <compare>
#include <type_traits>

namespace erbsland::text::impl {

/// A string-keyed unordered set with Erbsland string-list helpers.
/// @tparam tString The string key type.
/// @tparam tCaseInsensitive Use case-insensitive key hashing and equality.
/// @tested{StringSetTest}
template <typename tString, bool tCaseInsensitive>
class StringHashSet : public util::HashSet<
                          tString,
                          std::conditional_t<tCaseInsensitive, StringCIHash<tString>, StringHash<tString>>,
                          std::conditional_t<tCaseInsensitive, StringCIEqual<tString>, StringEqual<tString>>,
                          StringHashSet<tString, tCaseInsensitive>> {
public:
    using Hash = std::conditional_t<tCaseInsensitive, StringCIHash<tString>, StringHash<tString>>;
    using Equal = std::conditional_t<tCaseInsensitive, StringCIEqual<tString>, StringEqual<tString>>;
    using Base = util::HashSet<tString, Hash, Equal, StringHashSet<tString, tCaseInsensitive>>;
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
    StringHashSet(std::initializer_list<Key> values) {
        for (const auto &key : values) {
            insert(key);
        }
    }
    /// Creates a set by copying its raw representation.
    /// @param raw The raw set to copy.
    explicit StringHashSet(const Raw &raw) {
        for (const auto &key : raw) {
            insert(key);
        }
    }
    /// Creates a set from its raw representation.
    /// @param raw The raw set to move keys from.
    explicit StringHashSet(Raw &&raw) {
        for (const auto &key : raw) {
            insert(key);
        }
    }

    // defaults
    StringHashSet() = default;
    ~StringHashSet() = default;
    StringHashSet(const StringHashSet &) noexcept = default;
    StringHashSet(StringHashSet &&) noexcept = default;
    auto operator=(const StringHashSet &) noexcept -> StringHashSet & = default;
    auto operator=(StringHashSet &&) noexcept -> StringHashSet & = default;

public: // key changes
    /// Create a hash set from a list while compacting all keys.
    [[nodiscard]] static auto fromList(const util::List<Key> &values) -> StringHashSet {
        auto result = StringHashSet{};
        for (const auto &key : values) {
            result.insert(key);
        }
        return result;
    }
    /// Remove a string key.
    auto remove(const Key &key) -> StringHashSet & {
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
    [[nodiscard]] auto removed(const Key &key) const -> StringHashSet {
        auto result = *this;
        result.remove(key);
        return result;
    }
    /// Insert a string key.
    auto insert(const Key &key) -> StringHashSet & {
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
    [[nodiscard]] auto compareCI(const StringHashSet<tString, tOtherCaseInsensitive> &other) const -> bool {
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
