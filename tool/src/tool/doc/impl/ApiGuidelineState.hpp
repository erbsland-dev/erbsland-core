// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ItemIndex.hpp>
#include <erbsland/StringList.hpp>

namespace tool::doc::impl {

/// Structural context and accumulated diagnostics for one page.
struct ApiGuidelineState {
    /// Record a one-based source-line diagnostic.
    void issue(el::ItemIndex index, const el::String &message);

    int phase{};                  ///< Current position in the allowed section order.
    int typeCount{};              ///< Number of type sections.
    int coreCount{};              ///< Number of Core Semantics sections.
    int definitions{};            ///< Number of Pattern Definitions sections.
    el::ItemIndex sectionStart{}; ///< Start of the current section.
    el::String sectionKind;       ///< Current section grammar.
    int directives{};             ///< Code-block directives in the current section.
    int subsections{};            ///< Subsections within Core Semantics.
    el::StringList issues;        ///< Source-line diagnostics in discovery order.
};

}
