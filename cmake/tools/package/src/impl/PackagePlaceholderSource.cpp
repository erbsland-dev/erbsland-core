// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackagePlaceholderSource.hpp"

#include <erbsland/CharSet.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/placeholder/ReplacerError.hpp>
#include <erbsland/StringFormat.hpp>

namespace erbsland::package::impl {

using namespace text;
using namespace unit;
using namespace text::literals;

PackagePlaceholderSource::PackagePlaceholderSource(
    const PackageSettings &settings, const String &platform, const String &architecture, const String &target) :
    _settings{settings}, _platform{platform}, _architecture{architecture}, _target{target} {
}

auto PackagePlaceholderSource::sourceNames() const -> StringList {
    return StringList{"version"_el, "sys"_el, "project"_el, "package"_el, "target"_el};
}

auto PackagePlaceholderSource::resolve(const String &source, const String &parameter) -> String {
    if (source == "version"_el) {
        if (parameter.isEmpty()) {
            return _settings.version;
        }
        const auto parts = StringList::fromSplit(_settings.version, CharSet{".-"_el}, ItemCount::infinite(), true);
        if (parameter == "major"_el && parts.count() >= ItemCount{1U}) {
            return parts.get(ItemIndex::zero());
        }
        if (parameter == "minor"_el && parts.count() >= ItemCount{2U}) {
            return parts.get(ItemIndex{1U});
        }
        if ((parameter == "patch"_el || parameter == "revision"_el) && parts.count() >= ItemCount{3U}) {
            return parts.get(ItemIndex{2U});
        }
        if (parameter == "build"_el && parts.count() >= ItemCount{4U}) {
            return parts.get(ItemIndex{3U});
        }
    }
    if (source == "sys"_el) {
        if (parameter == "platform"_el) {
            return _platform;
        }
        if (parameter == "architecture"_el) {
            return _architecture;
        }
    }
    if (source == "project"_el && parameter == "name"_el) {
        return _settings.projectName;
    }
    if (source == "package"_el && parameter == "name"_el) {
        return _settings.name;
    }
    if (source == "target"_el && parameter == "name"_el && !_target.isEmpty()) {
        return _target;
    }
    throw placeholder::ReplacerError{
        placeholder::ReplacerErrorCategory::ValueNotFound,
        StringFormat{"Unknown package placeholder: {}:{}"_el}.build(source, parameter)};
}

}
