// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ApplicationErrorContext.hpp"

#include "../../i18n/DisplayTextMap_fwd.hpp"
#include "../../text/TextDocument.hpp"

namespace erbsland::core::impl {

/// Build a diagnostic document for an application error.
/// @tested{ApplicationErrorTest}
class ApplicationErrorDocumentBuilder {
public:
    /// Create a new builder for the given application error context.
    /// @param context The context to build the document for.
    /// @param displayText The display text map to use, or nullptr for the default.
    explicit ApplicationErrorDocumentBuilder(
        const ApplicationErrorContext &context, const i18n::DisplayTextMapConstPtr &displayText = {}) noexcept :
        _context{context}, _displayText{displayText} {}

    // defaults
    ~ApplicationErrorDocumentBuilder() = default;
    ApplicationErrorDocumentBuilder(const ApplicationErrorDocumentBuilder &) = delete;
    ApplicationErrorDocumentBuilder(ApplicationErrorDocumentBuilder &&) = default;
    auto operator=(const ApplicationErrorDocumentBuilder &) = delete;
    auto operator=(ApplicationErrorDocumentBuilder &&) = delete;

public:
    /// Build the diagnostic document.
    [[nodiscard]] auto build() const -> text::TextDocument;

private:
    const ApplicationErrorContext &_context;
    i18n::DisplayTextMapConstPtr _displayText;
};

}
