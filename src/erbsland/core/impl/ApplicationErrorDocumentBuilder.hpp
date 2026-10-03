// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ApplicationErrorContext.hpp"

#include "../../text/TextDocument.hpp"

namespace erbsland::core::impl {

/// Build a diagnostic document for an application error.
/// @tested{ApplicationErrorTest}
class ApplicationErrorDocumentBuilder {
public:
    /// Create a new builder for the given application error context.
    /// @param context The context to build the document for.
    explicit ApplicationErrorDocumentBuilder(const ApplicationErrorContext &context) noexcept : _context{context} {}

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
};

}
