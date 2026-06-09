// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockString.hpp"
#include "ParagraphOptions.hpp"
#include "Terminal_fwd.hpp"
#include "TerminalOptionsRenderer_fwd.hpp"
#include "TerminalOptionsTheme.hpp"

#include "../options/impl/OptionDisplayModel.hpp"
#include "../options/OptionDisplayText.hpp"
#include "../options/OptionRendererBase.hpp"

#include <utility>

namespace erbsland::cterm {

/// Terminal renderer for option help, version text and errors.
/// @tested{TerminalOptionsRendererTest}
class TerminalOptionsRenderer final : public options::OptionRendererBase {
public:
    /// Create a renderer with the default terminal options theme.
    explicit TerminalOptionsRenderer(TerminalPtr terminal);
    /// Create a renderer with an explicit terminal options theme.
    TerminalOptionsRenderer(TerminalPtr terminal, TerminalOptionsTheme theme);
    /// Create a renderer with explicit terminal options theme and display text.
    TerminalOptionsRenderer(TerminalPtr terminal, TerminalOptionsTheme theme, options::OptionDisplayText displayText);

    // defaults
    ~TerminalOptionsRenderer() override = default;
    TerminalOptionsRenderer(const TerminalOptionsRenderer &) = default;
    auto operator=(const TerminalOptionsRenderer &) -> TerminalOptionsRenderer & = default;
    TerminalOptionsRenderer(TerminalOptionsRenderer &&) = default;
    auto operator=(TerminalOptionsRenderer &&) -> TerminalOptionsRenderer & = default;

public:
    /// Create a shared terminal options renderer.
    [[nodiscard]] static auto create(TerminalPtr terminal) -> TerminalOptionsRendererPtr;
    /// Create a shared terminal options renderer with an explicit theme.
    [[nodiscard]] static auto create(TerminalPtr terminal, TerminalOptionsTheme theme) -> TerminalOptionsRendererPtr;
    /// Create a shared terminal options renderer with explicit theme and display text.
    [[nodiscard]] static auto create(
        TerminalPtr terminal, TerminalOptionsTheme theme, options::OptionDisplayText displayText)
        -> TerminalOptionsRendererPtr;

public: // implement OptionRenderer
    void displayHelp(const options::OptionsPtr &options, text::StringView moduleName) override;
    void displayVersion(const options::OptionsPtr &options, text::StringView moduleName) override;
    void displayError(const options::OptionsPtr &options, const options::OptionErrorContext &errorContext) override;

public: // accessors
    /// Get the terminal used by the renderer.
    [[nodiscard]] auto terminal() const noexcept -> const TerminalPtr & { return _terminal; }
    /// Get the active terminal options theme.
    [[nodiscard]] auto theme() const noexcept -> const TerminalOptionsTheme & { return _theme; }
    /// Set the active terminal options theme.
    void setTheme(TerminalOptionsTheme theme) { _theme = std::move(theme); }

private:
    [[nodiscard]] auto requireTerminal() const -> TerminalPtr;
    [[nodiscard]] auto terminalWidth(Terminal &terminal) const noexcept -> int;
    [[nodiscard]] auto descriptionColumn(const std::vector<options::impl::OptionDisplayRow> &rows, int width) const
        -> int;
    [[nodiscard]] auto rowTitleWidth(const std::vector<options::impl::OptionDisplayRow> &rows) const noexcept -> int;
    [[nodiscard]] auto bodyParagraphOptions(int lineIndent = 0, int wrappedLineIndent = 0) const -> ParagraphOptions;
    [[nodiscard]] auto optionParagraphOptions(int descriptionColumnValue) const -> ParagraphOptions;
    [[nodiscard]] auto styledText(text::StringView text, BlockStyle style) const -> BlockString;
    [[nodiscard]] auto styledUsage(const options::impl::OptionDisplayModel &model) const -> BlockString;
    [[nodiscard]] auto styledRow(const options::impl::OptionDisplayRow &row) const -> BlockString;
    [[nodiscard]] auto styledLabel(text::StringView label, text::StringView value, BlockStyle valueStyle) const
        -> BlockString;
    void writeHelpText(Terminal &terminal, const options::OptionHelp &help, int width);
    void writeUsage(Terminal &terminal, const options::impl::OptionDisplayModel &model);
    void writeSection(
        Terminal &terminal,
        text::StringView title,
        const std::vector<options::impl::OptionDisplayRow> &rows,
        int width);
    void writeRows(
        Terminal &terminal, const std::vector<options::impl::OptionDisplayRow> &rows, int descriptionColumnValue);
    void writeRow(Terminal &terminal, const options::impl::OptionDisplayRow &row, int descriptionColumnValue);

private:
    TerminalPtr _terminal;       ///< The terminal used for rendering.
    TerminalOptionsTheme _theme; ///< The active theme.
};

}
