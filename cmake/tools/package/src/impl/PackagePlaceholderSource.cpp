// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackagePlaceholderSource.hpp"

#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/placeholder/ReplacerError.hpp>
#include <erbsland/unit/ItemCount.hpp>
#include <erbsland/unit/ItemIndex.hpp>

namespace erbsland::package::impl {

using namespace text::literals;

PackagePlaceholderSource::PackagePlaceholderSource(
    const PackageSettings &settings, const text::String &platform, const text::String &architecture,
    const text::String &target) :
    _settings{settings}, _platform{platform}, _architecture{architecture}, _target{target} {}

auto PackagePlaceholderSource::sourceNames() const -> text::StringList {
    return text::StringList{"version"_el, "sys"_el, "project"_el, "package"_el, "target"_el};
}

auto PackagePlaceholderSource::resolve(const text::String &source, const text::String &parameter) -> text::String {
    if (source == "version"_el) {
        if (parameter.isEmpty()) { return _settings.version; }
        const auto parts = text::StringList::fromSplit(
            _settings.version, text::CharSet{".-"_el}, unit::ItemCount::infinite(), true);
        if (parameter == "major"_el && parts.count() >= unit::ItemCount{1U}) {
            return parts.get(unit::ItemIndex::zero());
        }
        if (parameter == "minor"_el && parts.count() >= unit::ItemCount{2U}) {
            return parts.get(unit::ItemIndex{1U});
        }
        if ((parameter == "patch"_el || parameter == "revision"_el) &&
            parts.count() >= unit::ItemCount{3U}) {
            return parts.get(unit::ItemIndex{2U});
        }
        if (parameter == "build"_el && parts.count() >= unit::ItemCount{4U}) {
            return parts.get(unit::ItemIndex{3U});
        }
    }
    if (source == "sys"_el) {
        if (parameter == "platform"_el) { return _platform; }
        if (parameter == "architecture"_el) { return _architecture; }
    }
    if (source == "project"_el && parameter == "name"_el) { return _settings.projectName; }
    if (source == "package"_el && parameter == "name"_el) { return _settings.name; }
    if (source == "target"_el && parameter == "name"_el && !_target.isEmpty()) { return _target; }
    throw text::placeholder::ReplacerError{
        text::placeholder::ReplacerErrorCategory::ValueNotFound,
        text::StringFormat{"Unknown package placeholder: {}:{}"_el}.build(source, parameter)};
}

}
