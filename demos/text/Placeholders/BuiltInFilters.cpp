// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PlaceholderDemos.hpp"

#include <erbsland/text/placeholder/Replacer.hpp>
#include <erbsland/text/placeholder/ReplacerError.hpp>

namespace demo {

/// Clean and select values in a volcanic field note.
void transformFieldNote() {
    auto replacer = el::placeholder::Replacer{};
    replacer.setVariableSource(
        {{{"site"_el, u8"  Vulkan_Askja-draft  "_el},
            {"sample"_el, "obs-2026-basalt"_el},
            {"description"_el, u8"Vulkansk aktivitet ved Askja"_el},
            {"status"_el, ""_el}}});
    replacer.addTextFilters();

    el::io::printLine(
        replacer.replaceOrThrow("Site: ${var:site|trim|remove:text=-draft|replace:text=_,rep=-|lower}"_el));
    el::io::printLine(replacer.replaceOrThrow("Year: ${var:sample|slice:start=4,length=4}"_el));
    el::io::printLine(replacer.replaceOrThrow("Sample: ${var:sample|remove:side=front,length=9|upper:ascii}"_el));
    el::io::printLine(replacer.replaceOrThrow("Summary: ${var:description|safe:length=18}"_el));
    el::io::printLine(replacer.replaceOrThrow("Status: ${var:status|default:pending}"_el));
}

/// Escape a value and choose text based on its contents.
void chooseFieldNoteText() {
    auto replacer = el::placeholder::Replacer{};
    replacer.setVariableSource(
        {{{"description"_el, u8"<krater>Askja</krater>"_el}, {"state"_el, "reviewed"_el}, {"sample"_el, "A-104"_el}}});
    replacer.addTextFilters();

    el::io::printLine(replacer.replaceOrThrow("HTML: ${var:description|escape:format=html,amount=required}"_el));
    el::io::printLine(
        replacer.replaceOrThrow("Audience: ${var:state|if:contains=review,then=internal,else=public}"_el));
    el::io::printLine(replacer.replaceOrThrow("Sample: ${var:sample|error_if:empty}"_el));
}

/// Report an empty value during strict replacement.
void requireFieldNoteValue() {
    auto replacer = el::placeholder::Replacer{};
    replacer.setVariableSource({{{"sample"_el, ""_el}}});
    replacer.addTextFilters();

    try {
        [[maybe_unused]] const auto result = replacer.replaceOrThrow("Sample: ${var:sample|required}"_el);
    } catch (const el::placeholder::ReplacerError &error) {
        el::io::printLine("Required sample failed: "_el, error.reason());
    }
}

}
