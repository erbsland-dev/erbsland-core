// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/html/impl/HtmlTokenizer.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

using el::text::StringConverter;
using el::text::StringView;
using el::text::html::impl::HtmlToken;
using el::text::html::impl::HtmlTokenizer;
using el::text::html::impl::HtmlTokenType;

TESTED_TARGETS(HtmlTokenizer HtmlToken HtmlTokenType)
class HtmlTokenizerTest final : public el::UnitTest {
public:
    struct ExpectedAttribute final {
        StringView name;
        StringView value;
    };

    struct ExpectedToken final {
        HtmlTokenType type;
        StringView value;
        std::vector<ExpectedAttribute> attributes{};
        bool selfClosing{false};
    };

public:
    void testTokenizeParsesPlainTextAndSimpleTags() {
        using namespace el::text::literals;

        const auto tokens = tokenize("Hello <strong>world</strong>!"_el);

        requireTokensEqual(
            tokens,
            {
                {HtmlTokenType::Text, "Hello "_el},
                {HtmlTokenType::TagOpen, "strong"_el},
                {HtmlTokenType::Text, "world"_el},
                {HtmlTokenType::TagClose, "strong"_el},
                {HtmlTokenType::Text, "!"_el},
            });
    }

    void testTokenizeParsesAttributesAndSelfClosingTags() {
        using namespace el::text::literals;

        const auto tokens = tokenize(R"(<img alt="A &amp; B" src=test disabled />)"_el);

        requireTokensEqual(
            tokens,
            {
                {
                    HtmlTokenType::TagOpen,
                    "img"_el,
                    {
                        {"alt"_el, "A & B"_el},
                        {"src"_el, "test"_el},
                        {"disabled"_el, {}},
                    },
                    true,
                },
            });
    }

    void testTokenizeParsesDocTypeCommentsAndEntities() {
        using namespace el::text::literals;

        const auto tokens = tokenize("<!DOCTYPE html>Fish &amp; chips &#35; &#x41;<!-- note -->"_el);

        requireTokensEqual(
            tokens,
            {
                {HtmlTokenType::DocType, "html"_el},
                {HtmlTokenType::Text, "Fish & chips # A"_el},
                {HtmlTokenType::Comment, " note "_el},
            });
    }

    void testTokenizeParsesAttributesInOpeningTag() {
        using namespace el::text::literals;

        const auto tokens = tokenize(R"(<A ID="main" class='Hero' href="/go">)"_el);

        requireTokensEqual(
            tokens,
            {
                {
                    HtmlTokenType::TagOpen,
                    "A"_el,
                    {
                        {"ID"_el, "main"_el},
                        {"class"_el, "Hero"_el},
                        {"href"_el, "/go"_el},
                    },
                },
            });
    }

    void testTokenizeLeavesUnknownAndMalformedEntitiesAsLiteralText() {
        using namespace el::text::literals;

        const auto tokens = tokenize("Text &bogus; &#x110000; &#x;"_el);

        requireTokensEqual(tokens, {{HtmlTokenType::Text, "Text &bogus; &#x110000; &#x;"_el}});
    }

    void testTokenizeFallsBackToLiteralTextForMalformedOpenTags() {
        using namespace el::text::literals;

        const auto tokens = tokenize(R"(prefix <strong title="broken suffix)"_el);

        requireTokensEqual(
            tokens,
            {
                {HtmlTokenType::Text, "prefix "_el},
                {HtmlTokenType::Text, R"(<strong title="broken suffix)"_el},
            });
    }

    void testTokenizeFallsBackToLiteralTextForMalformedCloseTagsAndDeclarations() {
        using namespace el::text::literals;

        const auto tokens = tokenize("</><!not-html><!-- not closed"_el);

        requireTokensEqual(
            tokens,
            {
                {HtmlTokenType::Text, "</>"_el},
                {HtmlTokenType::Text, "<!not-html>"_el},
                {HtmlTokenType::Text, "<!-- not closed"_el},
            });
    }

    void testTokenizeKeepsDecodedEntitiesInMalformedTagFallback() {
        using namespace el::text::literals;

        const auto tokens = tokenize(R"(prefix <strong title="A &amp; B suffix)"_el);

        requireTokensEqual(
            tokens,
            {
                {HtmlTokenType::Text, "prefix "_el},
                {HtmlTokenType::Text, R"(<strong title="A & B suffix)"_el},
            });
    }

    void testTokenizeLeavesInvalidEntitiesInAttributeValuesAsLiteralText() {
        using namespace el::text::literals;

        const auto tokens = tokenize(R"(<a title="A &bogus; &#x110000; B">)"_el);

        requireTokensEqual(
            tokens,
            {
                {
                    HtmlTokenType::TagOpen,
                    "a"_el,
                    {
                        {"title"_el, "A &bogus; &#x110000; B"_el},
                    },
                },
            });
    }

    void testTokenizeYieldsConstructsIncrementally() {
        using namespace el::text::literals;

        auto tokenizer = HtmlTokenizer{"<p>one</p><broken title=\"x"_elv};
        auto generator = tokenizer.tokenize();

        auto token = generator.next();
        REQUIRE(token.has_value());
        REQUIRE_EQUAL(token->type, HtmlTokenType::TagOpen);
        REQUIRE_EQUAL(token->value, "p"_el);

        token = generator.next();
        REQUIRE(token.has_value());
        REQUIRE_EQUAL(token->type, HtmlTokenType::Text);
        REQUIRE_EQUAL(token->value, "one"_el);

        token = generator.next();
        REQUIRE(token.has_value());
        REQUIRE_EQUAL(token->type, HtmlTokenType::TagClose);
        REQUIRE_EQUAL(token->value, "p"_el);

        token = generator.next();
        REQUIRE(token.has_value());
        REQUIRE_EQUAL(token->type, HtmlTokenType::Text);
        REQUIRE_EQUAL(token->value, R"(<broken title="x)"_el);
    }

private:
    [[nodiscard]] static auto tokenize(StringView text) -> std::vector<HtmlToken> {
        auto result = std::vector<HtmlToken>{};
        auto tokenizer = HtmlTokenizer{text};
        for (auto token : tokenizer.tokenize()) {
            result.push_back(std::move(token));
        }
        return result;
    }

    void requireTokensEqual(const std::vector<HtmlToken> &actual, const std::initializer_list<ExpectedToken> expected) {
        REQUIRE_EQUAL(actual.size(), expected.size());
        auto index = std::size_t{0};
        for (const auto &expectedToken : expected) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    REQUIRE_EQUAL(actual[index].type, expectedToken.type);
                    REQUIRE_EQUAL(actual[index].value, expectedToken.value);
                    REQUIRE_EQUAL(actual[index].selfClosing, expectedToken.selfClosing);
                    REQUIRE_EQUAL(actual[index].attributes.size(), expectedToken.attributes.size());
                    auto attributeIndex = std::size_t{0};
                    for (const auto &expectedAttribute : expectedToken.attributes) {
                        REQUIRE_EQUAL(actual[index].attributes[attributeIndex].name(), expectedAttribute.name);
                        REQUIRE_EQUAL(actual[index].attributes[attributeIndex].value(), expectedAttribute.value);
                        attributeIndex += 1;
                    }
                },
                [&]() -> std::string {
                    return "index = " + std::to_string(index) +
                        " / actualValue = " + StringConverter{actual[index].value}.toStdString();
                });
            index += 1;
        }
    }
};
