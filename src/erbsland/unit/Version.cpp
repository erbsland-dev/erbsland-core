// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Version.hpp"

#include "../text/StringBuilder.hpp"

namespace erbsland::unit {

auto Version::toString(const VersionPart precision) const -> text::String {
    auto builder = text::StringBuilder{};
    builder.appendInteger(_major.toRawValue());
    if (precision == VersionPart::Major) {
        return builder.toString();
    }
    builder.append(U'.');
    builder.appendInteger(_minor.toRawValue());
    if (precision == VersionPart::Minor) {
        return builder.toString();
    }
    builder.append(U'.');
    builder.appendInteger(_revision.toRawValue());
    if (precision == VersionPart::Revision) {
        return builder.toString();
    }
    builder.append(U'.');
    builder.appendInteger(_build.toRawValue());
    return builder.toString();
}

}
