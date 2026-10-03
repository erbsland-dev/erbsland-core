// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DisplayTextData.hpp"

#include "../text/CharSet.hpp"

#include <algorithm>

namespace erbsland::i18n {

using namespace text;

auto DisplayTextData::findText(const String &key) const -> std::optional<String> {
    auto domain = String{};
    auto name = key;
    if (const auto parts = key.splitAtLast(CharSet{U'.'})) {
        domain = parts->first;
        name = parts->second;
    }
    const auto domainName = [](const impl::DisplayTextDomain &value) -> String { return String{value.name}; };
    const auto entryKey = [](const impl::DisplayTextDomainEntry &value) -> String { return String{value.key}; };

    const auto data = domains();
    const auto foundDomain = std::ranges::lower_bound(data, domain, {}, domainName);
    if (foundDomain == data.end() || domainName(*foundDomain) != domain) {
        return {};
    }
    const auto entries = foundDomain->entries;
    const auto entry = std::ranges::lower_bound(entries, name, {}, entryKey);
    if (entry == entries.end() || entryKey(*entry) != name) {
        return {};
    }
    return String{entry->text};
}

}
