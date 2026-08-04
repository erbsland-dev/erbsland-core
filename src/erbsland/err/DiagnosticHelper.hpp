// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Diagnostic.hpp"
#include "Exception_fwd.hpp"

#include "../i18n/DisplayTextMap_fwd.hpp"
#include "../text/TextDocument_fwd.hpp"

#include <exception>

namespace erbsland::err {

/// A helper class to generate diagnostic documents.
class DiagnosticHelper {
private:
    constexpr static auto cMaximumCauseDepth = std::size_t{32U};

public:
    /// Create a helper for one exception and optional display-text map.
    /// @param error The exception to convert.
    /// @param displayText The display texts, or the English defaults if null.
    explicit DiagnosticHelper(const std::exception &error, const i18n::DisplayTextMapConstPtr &displayText = {});

    // defaults
    ~DiagnosticHelper() = default;
    DiagnosticHelper(const DiagnosticHelper &) = delete;
    DiagnosticHelper(DiagnosticHelper &&) noexcept = default;
    auto operator=(const DiagnosticHelper &) -> DiagnosticHelper & = delete;
    auto operator=(DiagnosticHelper &&) noexcept -> DiagnosticHelper & = delete;

public:
    /// Create a diagnostic for one Erbsland exception.
    /// @return The diagnostic for the exception itself.
    [[nodiscard]] auto toDiagnostic() -> DiagnosticConstPtr;

    /// Create a diagnostic document from an Erbsland exception, including causes.
    /// @return The diagnostic document.
    [[nodiscard]] auto toDocument() -> text::TextDocument;

public: // helper
    /// Create a diagnostic document from an exception pointer.
    /// @param errorPtr The exception pointer to convert.
    /// @param displayText The display texts, or the English defaults if null.
    /// @return The diagnostic document.
    [[nodiscard]] static auto documentFromError(
        const std::exception_ptr &errorPtr, const i18n::DisplayTextMapConstPtr &displayText = {}) -> text::TextDocument;

private:
    /// Append a diagnostic cause document.
    void appendCauseDocument(text::TextDocument &document, const DiagnosticConstPtr &diagnostic);
    /// Append the nested causes of an exception.
    void appendCauses(text::TextDocument &document, std::exception_ptr cause, std::size_t depth);

private:
    const std::exception &_error;
    i18n::DisplayTextMapConstPtr _displayText;
};

}
