// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>

namespace demo {

/// Prepare and inspect named values for one rendered document.
///
/// A context holds the names a layout can read. Setting an existing name replaces its value; testing a name before
/// reading it distinguishes an absent entry from an explicitly stored null value.
/// @notest{Demo function verified by the documentation executable.}
void contextValues() {
    auto context = el::render::Context{};
    context.set("region"_el, "sierra"_el).set("elevation"_el, 1840);

    // Replace a reading and inspect the prepared values before rendering.
    context.set("elevation"_el, 1920);
    el::io::printLine("Region: "_el, context.get("region"_el).asText());
    el::io::printLine("Elevation: "_el, context.get("elevation"_el).asInteger(), " m"_el);
    el::io::printLine("Named values: "_el, context.values().count().toSizeT());

    // An unknown name returns null; contains() tells whether a name was supplied at all.
    if (!context.contains("slope"_el)) {
        el::io::printLine("No slope classification was supplied."_el);
    }
    el::io::printLine("Missing value is null: "_el, context.get("slope"_el).isNull() ? "yes"_el : "no"_el);
}

}
