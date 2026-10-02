// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Distinguish chronological ordering from equality that retains display metadata.
/// @notest{Compiled and executed documentation demo.}
void compare() {
    const auto local = el::DateTime::fromIsoStringOrThrow("2026-07-01T14:30:00+03:00"_el);
    const auto utc = local.toUtc();
    el::io::printLine(el::StringFormat{"Local: {}; UTC: {}"_el}.build(local, utc));
    el::io::printLine(
        el::StringFormat{"Same ordering position: {}; same object value: {}; equal after UTC conversion: {}"_el}.build(
            (local <=> utc) == std::strong_ordering::equal, local == utc, local.toUtc() == utc));
    const auto later = utc.addedOrThrow(el::Duration{el::Seconds{1}});
    el::io::printLine(
        el::StringFormat{"Local instant before later UTC: {}; invalid sorts first: {}; invalid values equal: {}"_el}
            .build(local < later, el::DateTime{} < utc, el::DateTime{} == el::DateTime{}));
}

}
