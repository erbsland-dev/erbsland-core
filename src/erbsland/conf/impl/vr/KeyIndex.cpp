// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyIndex.hpp"

#include "../utilities/InternalError.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::conf::impl {

using namespace text::literals;

namespace {

template <text::CaseSensitivity::Value tCaseSensitivity>
struct KeyHash {
    auto operator()(const ConfKey &key) const noexcept -> std::size_t { return key.hash(tCaseSensitivity); }
};

using KeyHashCaseInsensitive = KeyHash<text::CaseSensitivity::CaseInsensitive>;
using KeyHashCaseSensitive = KeyHash<text::CaseSensitivity::CaseSensitive>;

template <text::CaseSensitivity::Value tCaseSensitivity>
struct KeyElementHash {
    auto operator()(const text::String &element) const noexcept -> std::size_t {
        return ConfKey::elementHash(element, tCaseSensitivity);
    }
};

using KeyElementHashCaseInsensitive = KeyElementHash<text::CaseSensitivity::CaseInsensitive>;
using KeyElementHashCaseSensitive = KeyElementHash<text::CaseSensitivity::CaseSensitive>;

template <text::CaseSensitivity::Value tCaseSensitivity>
struct KeyEqual {
    auto operator()(const ConfKey &lhs, const ConfKey &rhs) const noexcept -> bool {
        return lhs.isEqual(rhs, tCaseSensitivity);
    }
};

using KeyEqualCaseInsensitive = KeyEqual<text::CaseSensitivity::CaseInsensitive>;
using KeyEqualCaseSensitive = KeyEqual<text::CaseSensitivity::CaseSensitive>;

template <text::CaseSensitivity::Value tCaseSensitivity>
struct KeyElementEqual {
    auto operator()(const text::String &lhs, const text::String &rhs) const noexcept -> bool {
        return lhs.compare(rhs, text::CaseSensitivity{tCaseSensitivity}.asciiComparisonFn()) ==
            std::strong_ordering::equal;
    }
};

using KeyElementEqualCaseInsensitive = KeyElementEqual<text::CaseSensitivity::CaseInsensitive>;
using KeyElementEqualCaseSensitive = KeyElementEqual<text::CaseSensitivity::CaseSensitive>;

template <typename tKeyHash, typename tKeyEqual>
class KeyIndexDataSingle : public KeyIndexData {
public:
    [[nodiscard]] auto hasKey(const ConfKey &key) const noexcept -> bool override { return _keys.contains(key); }
    [[nodiscard]] auto hasKeyElement(const text::String &element, const std::size_t index) const noexcept
        -> bool override {
        if (index > 0) {
            return false;
        }
        return hasKey(ConfKey{element});
    }
    auto tryAddKey(const ConfKey &key) -> bool override { return _keys.insert(key).second; }

private:
    std::unordered_set<ConfKey, tKeyHash, tKeyEqual> _keys;
};

using KeyIndexDataSingleCaseInsensitive = KeyIndexDataSingle<KeyHashCaseInsensitive, KeyEqualCaseInsensitive>;
using KeyIndexDataSingleCaseSensitive = KeyIndexDataSingle<KeyHashCaseSensitive, KeyEqualCaseSensitive>;

template <typename tKeyHash, typename tKeyEqual, typename tKeyElementHash, typename tKeyElementEqual>
class KeyIndexDataMultiple : public KeyIndexData {
public:
    explicit KeyIndexDataMultiple(const std::size_t elementCount) : _keysByElement(elementCount) {}

    [[nodiscard]] auto hasKey(const ConfKey &key) const noexcept -> bool override { return _keys.contains(key); }
    [[nodiscard]] auto hasKeyElement(const text::String &element, const std::size_t index) const noexcept
        -> bool override {
        if (index >= _keysByElement.size()) {
            return false;
        }
        return _keysByElement[index].contains(element);
    }
    auto tryAddKey(const ConfKey &key) -> bool override {
        if (_keys.insert(key).second) {
            for (std::size_t i = 0; i < _keysByElement.size(); ++i) {
                _keysByElement[i].insert(key.element(i));
            }
            return true;
        }
        return false;
    }

private:
    std::unordered_set<ConfKey, tKeyHash, tKeyEqual> _keys;
    std::vector<std::unordered_set<text::String, tKeyElementHash, tKeyElementEqual>> _keysByElement;
};

using KeyIndexDataMultipleCaseInsensitive = KeyIndexDataMultiple<
    KeyHashCaseInsensitive,
    KeyEqualCaseInsensitive,
    KeyElementHashCaseInsensitive,
    KeyElementEqualCaseInsensitive>;
using KeyIndexDataMultipleCaseSensitive = KeyIndexDataMultiple<
    KeyHashCaseSensitive,
    KeyEqualCaseSensitive,
    KeyElementHashCaseSensitive,
    KeyElementEqualCaseSensitive>;

}

KeyIndex::KeyIndex(Name name, const text::CaseSensitivity caseSensitivity, const std::size_t elementCount) :
    _name{std::move(name)}, _caseSensitivity{caseSensitivity}, _elementCount{elementCount} {

    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(elementCount > 0, "The element count must be greater than zero");
    if (_elementCount == 1) {
        if (_caseSensitivity == text::CaseSensitivity::CaseInsensitive) {
            _data = std::make_unique<KeyIndexDataSingleCaseInsensitive>();
        } else {
            _data = std::make_unique<KeyIndexDataSingleCaseSensitive>();
        }
    } else {
        if (_caseSensitivity == text::CaseSensitivity::CaseInsensitive) {
            _data = std::make_unique<KeyIndexDataMultipleCaseInsensitive>(_elementCount);
        } else {
            _data = std::make_unique<KeyIndexDataMultipleCaseSensitive>(_elementCount);
        }
    }
}

auto KeyIndex::tryAddKey(const ConfKey &key) -> bool {
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(key.size() == _elementCount, "The key must have the correct size");
    return _data->tryAddKey(key);
}

auto KeyIndex::hasKey(const text::String &keyString) const noexcept -> bool {
    static const auto separatorCharacters = text::CharSet{U','};
    if (_elementCount > 1) {
        auto key = ConfKey{text::StringList::fromSplit(
            keyString, separatorCharacters, unit::ElementCount::fromSizeT(_elementCount - 1), true)};
        if (key.size() != _elementCount) {
            return false;
        }
        return hasKey(key);
    }
    return hasKey(ConfKey{keyString});
}

auto KeyIndex::hasKey(const ConfKey &key) const noexcept -> bool {
    return _data->hasKey(key);
}

auto KeyIndex::hasKey(const text::String &keyString, const std::size_t index) const noexcept -> bool {
    if (index >= _elementCount) {
        return false;
    }
    if (_elementCount == 1) {
        if (index == 0) {
            return hasKey(ConfKey{keyString});
        }
        return false;
    }
    return _data->hasKeyElement(keyString, index);
}

}
