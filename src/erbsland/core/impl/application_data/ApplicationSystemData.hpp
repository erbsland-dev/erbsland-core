// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationSystemData_fwd.hpp"

#include "../../../i18n/DisplayTextMap_fwd.hpp"
#include "../../../system/UserLookup_fwd.hpp"

#include <mutex>

namespace erbsland::core::impl {

/// Display text and operating-system lookup services shared by an application.
/// @tested{ApplicationLogTest ApplicationOptionsTest}
class ApplicationSystemData final {
public:
    // defaults
    ApplicationSystemData();
    ~ApplicationSystemData();

public:
    /// Access the display-text map, creating the default map on first use.
    [[nodiscard]] auto displayText() -> const i18n::DisplayTextMapConstPtr &;
    /// Replace the display-text map, restoring the default map for a null pointer.
    void setDisplayText(const i18n::DisplayTextMapConstPtr &displayText);
    /// Access the user lookup service, creating it on first use.
    [[nodiscard]] auto userLookup() -> system::UserLookup &;

private:
    std::mutex _mutex;                         ///< Serializes service creation and replacement.
    i18n::DisplayTextMapConstPtr _displayText; ///< Shared application display texts.
    system::UserLookupPtr _userLookup;         ///< Shared user and group lookup service.
};

}
