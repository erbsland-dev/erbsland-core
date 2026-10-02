// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Query the bundled rule version and supported names when offering time-zone choices.
/// Database versions and zone lists are runtime results rather than permanent application constants.
/// @notest{Compiled and executed documentation demo.}
void database() {
    const auto names = el::TimeZone::names();
    el::io::printLine(
        el::StringFormat{"Bundled rule version: {}\nSupported names: {}"_el}.build(
            el::TimeZone::databaseVersion(), names.count().toRawValue()));
    const auto alias = el::TimeZone::fromNameOrThrow("US/Eastern"_el);
    el::io::printLine(el::StringFormat{"Alias US/Eastern has primary name: {}"_el}.build(alias.name()));
    const auto requested = "Europe/Athens"_el;
    el::io::printLine(el::StringFormat{"Requested name listed: {}"_el}.build(!names.findFirst(requested).isNoIndex()));
}

}
