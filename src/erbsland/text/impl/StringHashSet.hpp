// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

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
    using View = typename StringViewFor<tString>::Type;

public:
    using Base::Base;
    using Base::contains;
    using Base::insert;
    using Base::remove;
    using Base::removed;
    using Base::tryInsert;
    using Base::tryRemove;
    StringHashSet() = default;
    ~StringHashSet() = default;
    StringHashSet(const StringHashSet &) noexcept = default;
    StringHashSet(StringHashSet &&) noexcept = default;
    auto operator=(const StringHashSet &) noexcept -> StringHashSet & = default;
    auto operator=(StringHashSet &&) noexcept -> StringHashSet & = default;

public: // view key changes
    /// Remove a string view key.
    auto remove(const View &key) -> StringHashSet & {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator != data.end()) {
            data.erase(iterator);
        }
        return *this;
    }
    /// Try to remove a string view key.
    [[nodiscard]] auto tryRemove(const View &key) -> bool {
        auto &data = this->mutableRaw();
        const auto iterator = data.find(key);
        if (iterator == data.end()) {
            return false;
        }
        data.erase(iterator);
        return true;
    }
    /// Return a set with a string view key removed.
    [[nodiscard]] auto removed(const View &key) const -> StringHashSet {
        auto result = *this;
        result.remove(key);
        return result;
    }
    /// Insert a string view key.
    auto insert(const View &key) -> StringHashSet & {
        static_cast<void>(tryInsert(key));
        return *this;
    }
    /// Try to insert a string view key.
    [[nodiscard]] auto tryInsert(const View &key) -> bool {
        auto &data = this->mutableRaw();
        if (data.find(key) != data.end()) {
            return false;
        }
        data.insert(tString{key});
        return true;
    }

public: // view key tests
    /// Test if the set contains a string view key.
    [[nodiscard]] auto contains(const View &key) const -> bool { return this->raw().find(key) != this->raw().end(); }

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
