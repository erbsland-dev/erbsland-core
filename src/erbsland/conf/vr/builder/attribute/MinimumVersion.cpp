// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MinimumVersion.hpp"

#include "../../../ConfError.hpp"
#include "../../../impl/vr/Rule.hpp"
#include "../../../impl/vr/VersionMask.hpp"

#include <limits>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void MinimumVersion::operator()(Rule &rule) {
    if (_version < 0) {
        throw conf::ConfError{ConfErrorCategory::Validation, "The minimum version must be non-negative"_el};
    }
    auto mask = impl::VersionMask::fromRanges({impl::ConfVersionRange{_version, std::numeric_limits<Integer>::max()}});
    if (_isNegated) {
        mask = !mask;
    }
    rule.limitVersionMask(mask);
}

}
