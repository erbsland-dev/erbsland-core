// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../i18n/DisplayTextMap_fwd.hpp"
#include "../text/String.hpp"
#include "../text/TextDocument_fwd.hpp"
#include "../unit/CodeLocation.hpp"

#include <memory>

namespace erbsland::err {

/// Abstract diagnostic interface for one error.
/// @tested{DiagnosticTest}
class Diagnostic : public std::enable_shared_from_this<Diagnostic> {
protected:
    Diagnostic() = default;

public:
    virtual ~Diagnostic() = default;
    Diagnostic(const Diagnostic &) = delete;
    Diagnostic(Diagnostic &&) = delete;
    auto operator=(const Diagnostic &) -> Diagnostic & = delete;
    auto operator=(Diagnostic &&) -> Diagnostic & = delete;

public:
    /// Get the source name, if this diagnostic points to one.
    [[nodiscard]] virtual auto sourceName() const noexcept -> text::String;
    /// Get the source path, if this diagnostic points to one.
    [[nodiscard]] virtual auto sourcePath() const noexcept -> text::String;
    /// Get the source location, if this diagnostic points to one.
    [[nodiscard]] virtual auto location() const noexcept -> unit::CodeLocation;

public: // conversion
    /// Render this diagnostic as plain text.
    [[nodiscard]] virtual auto toString() const noexcept -> text::String;
    /// Render this diagnostic as a structured text document.
    [[nodiscard]] virtual auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument;
    /// @overload
    [[nodiscard]] auto toTextDocument() const -> text::TextDocument;
};

/// Shared diagnostic pointer.
using DiagnosticPtr = std::shared_ptr<Diagnostic>;
/// Shared diagnostic pointer for immutable diagnostics.
using DiagnosticConstPtr = std::shared_ptr<const Diagnostic>;

}
