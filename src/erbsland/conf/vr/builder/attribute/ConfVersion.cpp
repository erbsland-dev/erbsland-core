// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfVersion.hpp"

#include "../../../ConfError.hpp"
#include "../../../impl/vr/Rule.hpp"
#include "../../../impl/vr/VersionMask.hpp"

#include <ranges>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void ConfVersion::operator()(Rule &rule) {
    auto mask = toVersionMask(_versions);
    if (_isNegated) {
        mask = !mask;
    }
    rule.limitVersionMask(mask);
}

auto ConfVersion::toVersionMask(const std::vector<Integer> &versions) -> impl::VersionMask {
    if (versions.empty()) {
        throw conf::ConfError{ConfErrorCategory::Validation, "The version list must not be empty"_el};
    }
    std::vector<Integer> uniqueVersions;
    uniqueVersions.reserve(versions.size());
    for (const auto version : versions) {
        if (version < 0) {
            throw conf::ConfError{ConfErrorCategory::Validation, "Versions must be non-negative integers"_el};
        }
        if (std::ranges::find(uniqueVersions, version) == uniqueVersions.end()) {
            uniqueVersions.push_back(version);
        }
    }
    return impl::VersionMask::fromIntegers(uniqueVersions);
}

}
