// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IndividualValuesDemos.hpp"

#include <erbsland/conf/Parser.hpp>

#include <cstdint>

namespace demo {

/// Convert one configuration value to its native C++ type.
///
/// The named `as...()` methods make the expected type visible, while `asType<T>()` is useful in generic code and for
/// converting an ELCL integer to a narrower native integer. The `OrThrow` forms report type and range mismatches.
void scalarConversions() {
    const auto configuration = "[survey]\n"
                               "forest: \"Mata Atlântica\"\n"
                               "maximum_observers: 18\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
    const auto forestValue = document->valueOrThrow("survey.forest"_el);
    const auto observerValue = document->valueOrThrow("survey.maximum_observers"_el);

    // Use a named conversion when the configuration type is known at the call site.
    const auto forest = forestValue->asTextOrThrow();

    // Template conversion can select a narrower native representation for an integer.
    const auto maximumObservers = observerValue->asTypeOrThrow<std::uint16_t>();
    el::io::printLine(forest, ": up to "_el, maximumObservers, " observers"_el);

    // A throwing conversion preserves a type mismatch instead of hiding it as a default value.
    try {
        (void)observerValue->asTextOrThrow();
    } catch (const el::conf::ConfError &) {
        el::io::printLine("An integer cannot be accessed as text."_el);
    }
}

}
