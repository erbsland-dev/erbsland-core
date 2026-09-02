// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/BlockStringTestHelper.hpp"

#include <erbsland/cterm/TerminalDocumentStyle.hpp>
#include <erbsland/cterm/TerminalDocumentStyleMarker.hpp>
#include <erbsland/text/TextNodeType.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <string>

namespace text = erbsland::text;

TESTED_TARGETS(TerminalDocumentStyle TerminalDocumentStyleMarker TerminalDocumentStyleSelector)
class TerminalDocumentStyleTest final : public UNITTEST_SUBCLASS(BlockStringTestHelper) {
public:
    void testStyleTokensAreNormalized() {
        const auto tokens = TerminalDocumentStyleSelector::splitStyleTokens(" beta alpha beta  key "_el);

        using TokenList = TerminalDocumentStyleSelector::TokenList;
        REQUIRE_EQUAL(tokens.count(), TokenList::Count{3U});
        REQUIRE_EQUAL(tokens.getRefOrThrow(TokenList::Index{0U}), "alpha"_el);
        REQUIRE_EQUAL(tokens.getRefOrThrow(TokenList::Index{1U}), "beta"_el);
        REQUIRE_EQUAL(tokens.getRefOrThrow(TokenList::Index{2U}), "key"_el);
    }

    void testCowCopyKeepsOriginalStyleUnchanged() {
        auto original = TerminalDocumentStyle{};
        auto copy = original;

        copy.setBaseTextStyle(BlockStyle{fg::Red});
        copy.edit(TerminalDocumentStyleSelector::paragraph()).setMargins(block::Margins{1});

        REQUIRE_NOT_EQUAL(original.baseTextStyle(), copy.baseTextStyle());
        REQUIRE_EQUAL(original.resolve(TerminalDocumentStyleSelector::paragraph()).margins(), block::Margins{0});
        REQUIRE_EQUAL(copy.resolve(TerminalDocumentStyleSelector::paragraph()).margins(), block::Margins{1});
    }

    void testRuleEditingResolvingAndErasing() {
        auto style = TerminalDocumentStyle{};
        const auto selector = TerminalDocumentStyleSelector::span({"beta"_el, "alpha"_el});

        style.edit(selector).setPrefix("["_el).setSuffix("]"_el);

        REQUIRE(style.definition(TerminalDocumentStyleSelector::span({"alpha"_el, "beta"_el})).has_value());

        const auto tokens = TerminalDocumentStyleSelector::splitStyleTokens("alpha beta"_el);
        const auto resolved = style.resolve(TerminalDocumentStyleSelector{text::TextNodeType::Span}, tokens);
        REQUIRE(resolved.prefix().has_value());
        REQUIRE(resolved.suffix().has_value());
        REQUIRE_EQUAL(render(*resolved.prefix()), std::string{"["});
        REQUIRE_EQUAL(render(*resolved.suffix()), std::string{"]"});

        style.erase(selector);
        REQUIRE(!style.definition(selector).has_value());
    }

    void testMarkerRenderingPreservesTabs() {
        auto literal = TerminalDocumentStyleMarker{};
        literal.setLiteral(BlockStringEditor{text::U32StringEditor{U"•\t"}});
        const auto renderedLiteral = literal.render(1U, BlockStyle{});

        REQUIRE_EQUAL(render(renderedLiteral), std::string{"•\t"});

        auto ordered = TerminalDocumentStyleMarker{};
        ordered.setOrdered();
        const auto renderedOrdered = ordered.render(7U, BlockStyle{});

        REQUIRE_EQUAL(render(renderedOrdered), std::string{"7.\t"});
    }

    void testAncestorSelectorMatchesAtAnyDepth() {
        auto style = TerminalDocumentStyle{};
        const auto selector = TerminalDocumentStyleSelector::descendantOf(
            text::TextNodeType::Heading, text::TextNodeType::Blockquote, {"error"_el});
        style.edit(selector).setTextStyle(BlockStyle{fg::Green});
        const auto tokens = TerminalDocumentStyleSelector::splitStyleTokens("error"_el);

        const auto withoutAncestor = style.resolve(TerminalDocumentStyleSelector{text::TextNodeType::Heading}, tokens);
        const auto withDeepAncestor = style.resolve(
            TerminalDocumentStyleSelector{text::TextNodeType::Heading},
            tokens,
            {text::TextNodeType::Document, text::TextNodeType::Section, text::TextNodeType::Blockquote});

        REQUIRE_NOT_EQUAL(withoutAncestor.textStyle(), BlockStyle{fg::Green});
        REQUIRE_EQUAL(withDeepAncestor.textStyle(), BlockStyle{fg::Green});
    }

    void testSystemOutputOptionEpilogMargins() {
        const auto &style = TerminalDocumentStyle::defaultSystemOutput();
        const auto tokens = TerminalDocumentStyleSelector::splitStyleTokens("option-epilog"_el);
        const auto rule = style.resolve(TerminalDocumentStyleSelector{text::TextNodeType::Paragraph}, tokens);

        REQUIRE_EQUAL(rule.margins(), (block::Margins{1, 0, 1, 0}));
    }

    void testSystemOutputCauseHeadingIsSecondaryFrameText() {
        const auto &style = TerminalDocumentStyle::defaultSystemOutput();
        const auto tokens = TerminalDocumentStyleSelector::splitStyleTokens("diagnostic-cause"_el);
        const auto rule =
            style.resolve(TerminalDocumentStyleSelector{text::TextNodeType::Heading, std::optional<int>{2}}, tokens);

        REQUIRE_EQUAL(rule.textStyle().fg(), fg::BrightBlack);
        REQUIRE_FALSE(rule.textStyle().attributes().isBold());
    }
};
