// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DisplayText.hpp"

#include "../core/impl/application_data/ApplicationSystemData.hpp"
#include "../core/impl/ApplicationDataAccess.hpp"

namespace erbsland::i18n {

using namespace text;

auto displayText(const String &key) -> String {
    const auto displayTexts = core::impl::applicationData()->system().get()->displayText();
    return displayTexts->text(key);
}

auto displayFormat(const String &key) -> StringFormat {
    const auto displayTexts = core::impl::applicationData()->system().get()->displayText();
    return displayTexts->format(key);
}

}
