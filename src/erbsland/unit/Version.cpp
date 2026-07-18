// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Version.hpp"

#include "../text/String.hpp"

namespace erbsland::unit {

using text::String;

auto Version::toString(const VersionPart precision) const -> String {
    const auto major = String::fromInteger(_major.toRawValue());
    if (precision == VersionPart::Major) {
        return major;
    }
    const auto minor = String::fromInteger(_minor.toRawValue());
    if (precision == VersionPart::Minor) {
        return String::fromJoined({major, String{"."}, minor});
    }
    const auto revision = String::fromInteger(_revision.toRawValue());
    if (precision == VersionPart::Revision) {
        return String::fromJoined({major, String{"."}, minor, String{"."}, revision});
    }
    return String::fromJoined(
        {major, String{"."}, minor, String{"."}, revision, String{"."}, String::fromInteger(_build.toRawValue())});
}

}
