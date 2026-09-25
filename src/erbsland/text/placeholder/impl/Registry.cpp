// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Registry.hpp"

#include "Name.hpp"

#include "../ReplacerError.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../Literals.hpp"
#include "../../StringFormat.hpp"

#include <algorithm>

namespace erbsland::text::placeholder::impl {

using namespace literals;

auto Registry::normalizeProviderName(const String &name, const String &kind) -> String {
    try {
        return normalizeName(name);
    } catch (const ReplacerError &) {
        throw err::LogicError{StringFormat{"Invalid placeholder {} name: {:/config_test}"_el}.build(kind, name)};
    }
}

void Registry::verifySourceName(const String &name, const std::vector<String> &pending) const {
    if (std::ranges::any_of(_sources, [&](const SourceEntry &entry) -> bool { return entry.name == name; }) ||
        std::ranges::find(pending, name) != pending.end()) {
        throw err::LogicError{StringFormat{"Placeholder source name is already registered: {}"_el}.build(name)};
    }
}

void Registry::verifyFilterName(const String &name, const std::vector<String> &pending) const {
    if (std::ranges::any_of(_filters, [&](const FilterEntry &entry) -> bool { return entry.name == name; }) ||
        std::ranges::find(pending, name) != pending.end()) {
        throw err::LogicError{StringFormat{"Placeholder filter name is already registered: {}"_el}.build(name)};
    }
}

auto Registry::source(const String &name) const noexcept -> SourcePtr {
    const auto iterator = std::ranges::find(_sources, name, &SourceEntry::name);
    return iterator != _sources.end() ? iterator->provider : nullptr;
}

auto Registry::filter(const String &name) const noexcept -> FilterPtr {
    const auto iterator = std::ranges::find(_filters, name, &FilterEntry::name);
    return iterator != _filters.end() ? iterator->provider : nullptr;
}

void Registry::addSource(const SourcePtr &source) {
    if (source == nullptr) {
        throw err::ParameterError{"Placeholder source cannot be null."_el, "source"_el};
    }
    const auto names = source->sourceNames();
    if (names.isEmpty()) {
        throw err::LogicError{"A placeholder source must announce at least one source name."};
    }
    auto normalizedNames = std::vector<String>{};
    normalizedNames.reserve(names.count().toSizeT());
    for (const auto &name : names) {
        auto normalized = normalizeProviderName(name, "source"_el);
        verifySourceName(normalized, normalizedNames);
        normalizedNames.emplace_back(std::move(normalized));
    }
    for (auto &name : normalizedNames) {
        _sources.emplace_back(SourceEntry{std::move(name), source});
    }
}

void Registry::removeSource(const SourcePtr &source) noexcept {
    std::erase_if(_sources, [&](const SourceEntry &entry) -> bool { return entry.provider == source; });
}

void Registry::addFilter(const FilterPtr &filter) {
    if (filter == nullptr) {
        throw err::ParameterError{"Placeholder filter cannot be null."_el, "filter"_el};
    }
    const auto names = filter->filterNames();
    if (names.isEmpty()) {
        throw err::LogicError{"A placeholder filter must announce at least one filter name."};
    }
    auto normalizedNames = std::vector<String>{};
    normalizedNames.reserve(names.count().toSizeT());
    for (const auto &name : names) {
        auto normalized = normalizeProviderName(name, "filter"_el);
        verifyFilterName(normalized, normalizedNames);
        normalizedNames.emplace_back(std::move(normalized));
    }
    for (auto &name : normalizedNames) {
        _filters.emplace_back(FilterEntry{std::move(name), filter});
    }
}

void Registry::removeFilter(const FilterPtr &filter) noexcept {
    std::erase_if(_filters, [&](const FilterEntry &entry) -> bool { return entry.provider == filter; });
}

auto Registry::resolve(const String &name, const String &parameter) const -> String {
    const auto iterator = std::ranges::find(_sources, name, &SourceEntry::name);
    if (iterator == _sources.end()) {
        throw ReplacerError{
            ReplacerErrorCategory::Unsupported, StringFormat{"Unknown placeholder source: {}"_el}.build(name)};
    }
    return iterator->provider->resolve(name, parameter);
}

auto Registry::apply(const String &name, const String &parameter, const String &value) const -> String {
    const auto iterator = std::ranges::find(_filters, name, &FilterEntry::name);
    if (iterator == _filters.end()) {
        throw ReplacerError{
            ReplacerErrorCategory::Unsupported, StringFormat{"Unknown placeholder filter: {}"_el}.build(name)};
    }
    return iterator->provider->apply(name, parameter, value);
}

}
