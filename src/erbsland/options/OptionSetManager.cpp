// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionSetManager.hpp"

#include "../text/String.hpp"

namespace erbsland::options {

auto OptionSetManager::addOption(const text::String &name) -> OptionEditor {
    return addOption({name});
}

}
