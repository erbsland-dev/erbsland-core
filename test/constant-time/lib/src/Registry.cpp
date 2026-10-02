// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Registry.hpp"

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/util/List.hpp>

namespace app::constant_time {

using namespace el::text::literals;

void Registry::add(Registration registration) {
    for (const auto &entry : _entries) {
        if (entry.id() == registration.id()) {
            throw el::ParameterError{"Duplicate timing-test ID."_el, "registration"_el};
        }
    }
    _entries.append(std::move(registration));
}

auto Registry::select(const el::StringList &ids, const bool all) const -> el::List<Registration> {
    if (all && !ids.isEmpty()) {
        throw el::ParameterError{"--all and --test cannot be combined."_el, "selection"_el};
    }
    for (const auto &id : ids) {
        auto found = false;
        for (const auto &entry : _entries) {
            found |= entry.id() == id;
        }
        if (!found) {
            throw el::ParameterError{el::StringFormat{"Unknown test: {}"_el}.build(id), "test"_el};
        }
    }
    auto selected = el::List<Registration>{};
    for (const auto &entry : _entries) {
        auto include = all;
        for (const auto &id : ids) {
            include |= entry.id() == id;
        }
        if (include) {
            selected.append(entry);
        }
    }
    return selected;
}

auto Registry::builtIn() -> Registry {
    auto registry = Registry{};
    registry.addComparisons();
    registry.addSymmetric();
    registry.addAuthentication();
    registry.addSigning();
    registry.addValidation();
    return registry;
}
}
