// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStyle.hpp"
#include "ParagraphIndents.hpp"
#include "TerminalDocumentStyleRule.hpp"
#include "TerminalDocumentStyleSelector.hpp"

#include "impl/TerminalDocumentStyleData.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace erbsland::cterm {

/// A selector-driven terminal document style sheet.
/// @tested{TerminalDocumentStyleTest}
class TerminalDocumentStyle final {
public:
    /// Shared predefined style variants.
    enum class Predefined : uint8_t {
        Plain,        ///< The plain built-in style.
        Simple,       ///< A compact colored style.
        Styled,       ///< A more decorative colored style.
        SystemOutput, ///< The default style for application system output.
    };

public:
    /// Create the default plain document style.
    TerminalDocumentStyle();

    // defaults
    ~TerminalDocumentStyle() = default;
    TerminalDocumentStyle(const TerminalDocumentStyle &) noexcept = default;
    TerminalDocumentStyle(TerminalDocumentStyle &&) noexcept = default;
    auto operator=(const TerminalDocumentStyle &) noexcept -> TerminalDocumentStyle & = default;
    auto operator=(TerminalDocumentStyle &&) noexcept -> TerminalDocumentStyle & = default;

public: // accessors
    /// Get the base text style.
    [[nodiscard]] auto baseTextStyle() const noexcept -> BlockStyle { return _data->baseTextStyle; }
    /// Replace the base text style.
    /// @param style The new base text style.
    void setBaseTextStyle(BlockStyle style);
    /// Get the base block layout.
    [[nodiscard]] auto baseBlockLayout() const noexcept -> const ParagraphIndents & { return _data->baseBlockLayout; }
    /// Replace the base block layout.
    /// @param layout The new base block layout.
    void setBaseBlockLayout(ParagraphIndents layout);

public:
    /// Access one exact stored rule definition.
    /// @param selector The selector to query.
    /// @return The stored rule, if present.
    [[nodiscard]] auto definition(const TerminalDocumentStyleSelector &selector) const noexcept
        -> std::optional<std::reference_wrapper<const TerminalDocumentStyleRule>>;
    /// Create or access one exact stored rule definition.
    /// New definitions are initialized from the currently resolved rule for the same selector.
    /// @param selector The selector to edit.
    /// @return A mutable reference to the stored rule.
    auto edit(const TerminalDocumentStyleSelector &selector) -> TerminalDocumentStyleRule &;
    /// Remove one exact stored rule definition.
    /// @param selector The selector to erase.
    void erase(const TerminalDocumentStyleSelector &selector) noexcept;
    /// Resolve the best matching rule.
    /// @param selector The requested selector.
    /// @param contextTokens Additional style tokens from the current node.
    /// @param ancestors Node types in the parent chain, from root to direct parent.
    /// @return The resolved rule.
    [[nodiscard]] auto resolve(
        const TerminalDocumentStyleSelector &selector,
        const TerminalDocumentStyleSelector::TokenList &contextTokens = {},
        const std::vector<text::TextNodeType> &ancestors = {}) const -> TerminalDocumentStyleRule;

public: // defaults
    /// Access a predefined default style.
    /// @param predefined The predefined style variant.
    [[nodiscard]] static auto defaultStyle(Predefined predefined = Predefined::Plain) noexcept
        -> const TerminalDocumentStyle &;
    /// Access the plain default style.
    [[nodiscard]] static auto defaultPlain() noexcept -> const TerminalDocumentStyle &;
    /// Access the simple default style.
    [[nodiscard]] static auto defaultSimple() noexcept -> const TerminalDocumentStyle &;
    /// Access the styled default style.
    [[nodiscard]] static auto defaultStyled() noexcept -> const TerminalDocumentStyle &;
    /// Access the application system-output default style.
    [[nodiscard]] static auto defaultSystemOutput() noexcept -> const TerminalDocumentStyle &;

private:
    /// Detach shared style data before modifying it.
    void detach();
    /// Initialize this style with plain built-in defaults.
    void initializePlainDefaults();
    /// Get the built-in default rule for a node type and optional level.
    [[nodiscard]] auto defaultRuleFor(text::TextNodeType nodeType, const std::optional<int> &level) const noexcept
        -> TerminalDocumentStyleRule;
    /// Find the mutable entry with an exact selector.
    [[nodiscard]] auto findEntry(const TerminalDocumentStyleSelector &selector) noexcept
        -> impl::TerminalDocumentStyleData::EntryList::Index;
    /// Find the immutable entry with an exact selector.
    [[nodiscard]] auto findEntry(const TerminalDocumentStyleSelector &selector) const noexcept
        -> impl::TerminalDocumentStyleData::EntryList::Index;
    /// Create the compact colored built-in style.
    [[nodiscard]] static auto createSimpleDefaultStyle() -> TerminalDocumentStyle;
    /// Create the decorative colored built-in style.
    [[nodiscard]] static auto createStyledDefaultStyle() -> TerminalDocumentStyle;
    /// Create the built-in system-output style.
    [[nodiscard]] static auto createSystemOutputDefaultStyle() -> TerminalDocumentStyle;
    /// Combine selector and contextual tokens without duplicates.
    [[nodiscard]] static auto combinedTokens(
        const TerminalDocumentStyleSelector &selector, const TerminalDocumentStyleSelector::TokenList &contextTokens)
        -> TerminalDocumentStyleSelector::TokenList;
    /// Test if all required tokens are included in contextual tokens.
    [[nodiscard]] static auto tokensMatch(
        const TerminalDocumentStyleSelector::TokenList &requiredTokens,
        const TerminalDocumentStyleSelector::TokenList &contextTokens) noexcept -> bool;

private:
    impl::TerminalDocumentStyleDataPtr _data; ///< Shared style data.
};

}
