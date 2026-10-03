// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationSystemData.hpp"

#include "../../../i18n/impl/DisplayText.hpp"
#include "../../../system/UserLookup.hpp"

#include <memory>
#include <utility>

namespace erbsland::core::impl {

ApplicationSystemData::ApplicationSystemData() = default;

ApplicationSystemData::~ApplicationSystemData() = default;

auto ApplicationSystemData::displayText() -> i18n::DisplayTextPtr {
    const auto lock = std::scoped_lock{_mutex};
    if (_displayText == nullptr) {
        _displayText = i18n::DisplayTextPtr{new i18n::impl::DisplayText{}};
    }
    return _displayText;
}

auto ApplicationSystemData::userLookup() -> system::UserLookup & {
    const auto lock = std::scoped_lock{_mutex};
    if (_userLookup == nullptr) {
        _userLookup = std::make_unique<system::UserLookup>();
    }
    return *_userLookup;
}

}
