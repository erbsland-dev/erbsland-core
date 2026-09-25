// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PlaceholderDemos.hpp"

#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/text/placeholder/Replacer.hpp>
#include <erbsland/text/placeholder/ReplacerError.hpp>

namespace demo {

/// Expand a gallery label with application values and a text filter.
///
/// `Replacer` reads a source name, its parameter, and optional filters from each expression. The variable source
/// supplies application text; filters process it from left to right. The surrounding text remains unchanged.
void replaceGalleryLabel() {
    auto replacer = el::placeholder::Replacer{};
    replacer.setVariableSource({{{"movement"_el, u8"Ιμπρεσιονισμός"_el}}});
    replacer.addTextFilters();

    const auto label = replacer.replaceOrThrow(u8"Gallery: ${var:movement} (${var:movement|upper})"_el);
    el::io::printLine(label);
}

/// Combine built-in application variables, environment values, and text filters.
///
/// The `var` source resolves a fixed map. The `env` source reads a process environment variable when replacement
/// runs. The `required` source flag reports an absent variable instead of returning `undefined`.
void useBuiltInProviders() {
    auto environment = el::system::EnvironmentVariables{};
    environment.setOrThrow("ERBSLAND_DEMO_ART_MOVEMENT"_el, u8"Ντανταϊσμός"_el);
    auto replacer = el::placeholder::Replacer{};
    replacer.setVariableSource({{{"movement"_el, u8"  Κυβισμός  "_el}}});
    replacer.addEnvironmentSource();
    replacer.addTextFilters();

    el::io::printLine(replacer.replaceOrThrow("Movement: ${var:movement|trim}"_el));
    el::io::printLine(replacer.replaceOrThrow("Next movement: ${env:ERBSLAND_DEMO_ART_MOVEMENT,required}"_el));
    environment.removeOrThrow("ERBSLAND_DEMO_ART_MOVEMENT"_el);
}

/// Change the placeholder frame and separators to fit a text format.
///
/// `ReplacerOptions` belongs to a replacer at construction. Empty separators disable the corresponding syntax:
/// without a name separator, providers receive an empty parameter; without a filter separator, no chain is parsed.
void chooseSyntax() {
    auto options = el::placeholder::ReplacerOptions{};
    options.setFrame("[["_el, "]]"_el).setNameSeparator("="_el).setFilterSeparator("/"_el);
    auto replacer = el::placeholder::Replacer{options};
    replacer.setVariableSource({{{"movement"_el, u8"Εξπρεσιονισμός"_el}}});
    replacer.addTextFilters();

    el::io::printLine(replacer.replaceOrThrow("[[var=movement/upper]]"_el));
}

/// Write a literal frame using backslash or doubled-delimiter escaping.
///
/// Escape mode changes how delimiters are quoted in the input. It does not escape the returned source text or
/// interpret it a second time.
void chooseEscapeMode() {
    auto backslash = el::placeholder::Replacer{};
    backslash.setVariableSource({{{"movement"_el, u8"Κυβισμός"_el}}});
    el::io::printLine(backslash.replaceOrThrow("Literal: \\${var:movement}; value: ${var:movement}"_el));

    auto options = el::placeholder::ReplacerOptions{};
    options.setEscapeMode(el::placeholder::EscapeMode::Double);
    auto doubled = el::placeholder::Replacer{options};
    doubled.setVariableSource({{{"movement"_el, u8"Κυβισμός"_el}}});
    el::io::printLine(doubled.replaceOrThrow("Literal: $${var:movement}; value: ${var:movement}"_el));
}

/// Choose strict replacement for a required label and tolerant replacement for an editable draft.
///
/// `validate()` checks syntax and provider parameters. `replace()` preserves a failed expression and keeps scanning;
/// `replaceOrThrow()` reports the first problem with a category and input offset.
void handleInvalidExpression() {
    auto replacer = el::placeholder::Replacer{};
    replacer.setVariableSource({{{"movement"_el, u8"Ρεαλισμός"_el}}});
    const auto draft = "${var:missing}; ${var:movement}"_el;

    el::io::printLine("Draft: "_el, replacer.replace(draft));
    try {
        [[maybe_unused]] const auto result = replacer.replaceOrThrow(draft);
    } catch (const el::placeholder::ReplacerError &error) {
        el::io::printLine("Strict replacement failed: "_el, error.reason());
    }
}

}
