// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationSystemData.hpp"

#include "../../../i18n/DisplayTextMap.hpp"
#include "../../../system/UserLookup.hpp"

#include <memory>
#include <utility>

namespace erbsland::core::impl {

ApplicationSystemData::ApplicationSystemData() = default;

ApplicationSystemData::~ApplicationSystemData() = default;

auto ApplicationSystemData::displayText() -> const i18n::DisplayTextMapConstPtr & {
    const auto lock = std::scoped_lock{_mutex};
    if (_displayText == nullptr) {
        _displayText = i18n::DisplayTextMap::defaultMap();
    }
    return _displayText;
}

void ApplicationSystemData::setDisplayText(const i18n::DisplayTextMapConstPtr &displayText) {
    const auto lock = std::scoped_lock{_mutex};
    _displayText = displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap();
}

auto ApplicationSystemData::userLookup() -> system::UserLookup & {
    const auto lock = std::scoped_lock{_mutex};
    if (_userLookup == nullptr) {
        _userLookup = std::make_unique<system::UserLookup>();
    }
    return *_userLookup;
}

}
