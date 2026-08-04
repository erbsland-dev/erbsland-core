// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MaximumVersion.hpp"

#include "../../../ConfError.hpp"
#include "../../../impl/vr/Rule.hpp"
#include "../../../impl/vr/VersionMask.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void MaximumVersion::operator()(Rule &rule) {
    if (_version < 0) {
        throw conf::ConfError{ConfErrorCategory::Validation, "The maximum version must be non-negative"_el};
    }
    auto mask = impl::VersionMask::fromRanges({impl::ConfVersionRange{0, _version}});
    if (_isNegated) {
        mask = !mask;
    }
    rule.limitVersionMask(mask);
}

}
