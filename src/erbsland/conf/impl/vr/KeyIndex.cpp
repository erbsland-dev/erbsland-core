// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyIndex.hpp"

#include "KeyIndexDataTypes.hpp"

#include "../utilities/InternalError.hpp"

#include <utility>

namespace erbsland::conf::impl {

KeyIndex::~KeyIndex() = default;

KeyIndex::KeyIndex(Name name, const text::CaseSensitivity caseSensitivity, const std::size_t elementCount) :
    _name{std::move(name)}, _caseSensitivity{caseSensitivity}, _elementCount{elementCount} {

    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(elementCount > 0, "The element count must be greater than zero"_el);
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
    ERBSLAND_CORE_CONF_REQUIRE_SAFETY(key.size() == _elementCount, "The key must have the correct size"_el);
    return _data->tryAddKey(key);
}

auto KeyIndex::hasKey(const text::String &keyString) const noexcept -> bool {
    static const auto separatorCharacters = text::CharSet{U','};
    if (_elementCount > 1) {
        auto key = ConfKey{text::StringList::fromSplit(
            keyString, separatorCharacters, unit::ItemCount::fromSizeT(_elementCount - 1), true)};
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
