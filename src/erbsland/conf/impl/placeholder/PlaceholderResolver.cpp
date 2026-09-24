// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PlaceholderResolver.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../ConfError.hpp"
#include "../../Name.hpp"

#include <algorithm>

namespace erbsland::conf::impl::placeholder {

using namespace text::literals;

auto PlaceholderResolver::normalizeProviderName(const text::String &name, const text::String &kind) -> text::String {
    try {
        auto normalized = Name::normalize(name);
        if (normalized.startsWith("@"_el)) {
            throw err::LogicError{text::StringFormat{"Placeholder {} names must not be meta names."_el}.build(kind)};
        }
        return normalized;
    } catch (const ConfError &) {
        throw err::LogicError{text::StringFormat{"Invalid placeholder {} name: {:/config_test}"_el}.build(kind, name)};
    }
}

void PlaceholderResolver::verifySourceName(const text::String &name, const std::vector<text::String> &pending) const {
    if (std::ranges::any_of(_sources, [&](const SourceEntry &entry) -> bool { return entry.name == name; }) ||
        std::ranges::find(pending, name) != pending.end()) {
        throw err::LogicError{text::StringFormat{"Placeholder source name is already registered: {}"_el}.build(name)};
    }
}

void PlaceholderResolver::verifyFilterName(const text::String &name, const std::vector<text::String> &pending) const {
    if (std::ranges::any_of(_filters, [&](const FilterEntry &entry) -> bool { return entry.name == name; }) ||
        std::ranges::find(pending, name) != pending.end()) {
        throw err::LogicError{text::StringFormat{"Placeholder filter name is already registered: {}"_el}.build(name)};
    }
}

auto PlaceholderResolver::source(const text::String &name) const noexcept -> PlaceholderSourcePtr {
    const auto iterator = std::ranges::find(_sources, name, &SourceEntry::name);
    return iterator != _sources.end() ? iterator->provider : nullptr;
}

void PlaceholderResolver::addSource(const PlaceholderSourcePtr &source) {
    if (source == nullptr) {
        throw err::ParameterError{"Placeholder source cannot be null."_el, "source"_el};
    }
    const auto names = source->sourceNames();
    if (names.isEmpty()) {
        throw err::LogicError{"A placeholder source must announce at least one source name."};
    }
    auto normalizedNames = std::vector<text::String>{};
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

void PlaceholderResolver::removeSource(const PlaceholderSourcePtr &source) noexcept {
    std::erase_if(_sources, [&](const SourceEntry &entry) -> bool { return entry.provider == source; });
}

void PlaceholderResolver::addFilter(const PlaceholderFilterPtr &filter) {
    if (filter == nullptr) {
        throw err::ParameterError{"Placeholder filter cannot be null."_el, "filter"_el};
    }
    const auto names = filter->filterNames();
    if (names.isEmpty()) {
        throw err::LogicError{"A placeholder filter must announce at least one filter name."};
    }
    auto normalizedNames = std::vector<text::String>{};
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

void PlaceholderResolver::removeFilter(const PlaceholderFilterPtr &filter) noexcept {
    std::erase_if(_filters, [&](const FilterEntry &entry) -> bool { return entry.provider == filter; });
}

auto PlaceholderResolver::resolve(const text::String &name, const text::String &parameter) const -> text::String {
    const auto iterator = std::ranges::find(_sources, name, &SourceEntry::name);
    if (iterator == _sources.end()) {
        throw ConfError{
            ConfErrorCategory::Unsupported, text::StringFormat{"Unknown placeholder source: {}"_el}.build(name)};
    }
    return iterator->provider->resolve(name, parameter);
}

auto PlaceholderResolver::apply(
    const text::String &name, const text::String &parameter, const text::String &value) const -> text::String {
    const auto iterator = std::ranges::find(_filters, name, &FilterEntry::name);
    if (iterator == _filters.end()) {
        throw ConfError{
            ConfErrorCategory::Unsupported, text::StringFormat{"Unknown placeholder filter: {}"_el}.build(name)};
    }
    return iterator->provider->apply(name, parameter, value);
}

}
