// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationTerminalData_fwd.hpp"

#include "../../../cterm/Terminal_fwd.hpp"
#include "../../../cterm/TerminalDocumentStyle.hpp"
#include "../../../stream/StandardStreamRedirect.hpp"
#include "../../../stream/TextOutputStream_fwd.hpp"
#include "../../../text/TextDocument_fwd.hpp"

namespace erbsland::core::impl {

/// Terminal integration, system-output rendering, and stream redirection for an application.
/// @tested{ApplicationLogTest ApplicationOptionsTest ApplicationTerminalTest}
class ApplicationTerminalData final {
public:
    /// Enable terminal integration with an initialized terminal.
    void enable(cterm::TerminalPtr terminal);
    /// Test whether terminal integration is enabled.
    [[nodiscard]] auto isEnabled() const noexcept -> bool;
    /// Access the configured terminal, which can be null when initialization failed.
    [[nodiscard]] auto terminal() const noexcept -> const cterm::TerminalPtr &;
    /// Access the style used for system output.
    [[nodiscard]] auto systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle &;
    /// Replace the style used for system output.
    void setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept;
    /// Render a system-output document to the best available target.
    void renderSystemOutput(const text::TextDocument &document);
    /// Flush output, restore the terminal, and release redirection state.
    void cleanup() noexcept;

private:
    /// Select the standard output stream for a plain-text document.
    [[nodiscard]] static auto plainSystemOutputStream(const text::TextDocument &document)
        -> stream::TextOutputStreamPtr;

private:
    bool _isEnabled{false};                                   ///< Whether terminal integration is enabled.
    cterm::TerminalPtr _terminal;                             ///< Configured terminal.
    cterm::TerminalDocumentStyle _systemOutputStyle{
        cterm::TerminalDocumentStyle::defaultSystemOutput()}; ///< Style for system-output documents.
    stream::StandardStreamRedirect _streamRedirect;           ///< Active standard-stream redirection.
};

}
