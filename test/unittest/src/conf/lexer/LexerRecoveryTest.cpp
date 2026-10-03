// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/conf/impl/decoder/TokenDecoder.hpp>
#include <erbsland/conf/impl/lexer/DocumentPart.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/placeholder/Source.hpp>
#include <erbsland/text/placeholder/impl/Registry.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(TokenDecoder Lexer)
class LexerRecoveryTest final : public el::UnitTest {
    class CountingSource final : public el::text::placeholder::Source {
    public: // implement Source
        auto sourceNames() const -> el::StringList override { return el::StringList{"count"_el}; }
        auto resolve(const el::String &, const el::String &) -> el::String override {
            ++calls;
            return "expanded"_el;
        }

    public:
        int calls{}; ///< Number of placeholder resolutions.
    };

private:
    auto decoder(
        const el::String &text,
        el::conf::impl::TokenDecoderMode mode,
        el::text::placeholder::impl::RegistryPtr registry = {}) -> el::conf::impl::TokenDecoderPtr {
        auto source = el::conf::Source::fromString(text);
        source->open();
        auto result =
            el::conf::impl::TokenDecoder::create(el::conf::impl::CharStream::create(source), std::move(registry), mode);
        result->initialize();
        return result;
    }

    auto read(const el::conf::impl::TokenDecoderPtr &decoder) -> el::List<el::conf::impl::LexerToken> {
        auto result = el::List<el::conf::impl::LexerToken>{};
        while (!decoder->character().isEndOfData()) {
            for (auto &&token : el::conf::impl::lexer::readDocumentPart(*decoder)) {
                result.append(std::move(token));
            }
        }
        return result;
    }

public:
    void testModesAgreeOnValidTokens() {
        using enum el::conf::impl::TokenDecoderMode;
        const auto source = "  # α\r\n[main]\r\nx: 0x12, true, \"hé\"\r\n"
                            "text: \"\"\"\r\n    contents\r\n    \"\"\" # end\r\n"
                            "bytes: <<<hex\n    12 ab\n    >>>\n"
                            "code: ```cpp\n    text\n    ```\n"
                            "regex: ///\n    .*\n    ///\n"
                            "list:\n    * 1, 2\n    * 3, 4"_el;
        const auto strict = decoder(source, Parsing);
        const auto syntax = decoder(source, Syntax);
        const auto expected = read(strict);
        const auto actual = read(syntax);
        REQUIRE_EQUAL(expected.count(), actual.count());
        for (auto i = el::ItemIndex{}; i.isWithin(expected.count()); ++i) {
            const auto &a = expected.getRef(i);
            const auto &b = actual.getRef(i);
            REQUIRE(a.type() == b.type());
            REQUIRE_EQUAL(a.rawText(), b.rawText());
            REQUIRE(a.begin() == b.begin());
            REQUIRE(a.end() == b.end());
        }
        REQUIRE(strict->takeDiagnostics().isEmpty());
        REQUIRE(syntax->takeDiagnostics().isEmpty());
    }

    void testStrictFailureAndSyntaxRecovery() {
        using enum el::conf::impl::TokenDecoderMode;
        const auto source = "[main]\nx: \"\"\"\n    bad\\q\n    valid\n    \"\"\"\ngood: 2\n"_el;
        const auto strict = decoder(source, Parsing);
        REQUIRE_THROWS_AS(el::conf::ConfError, read(strict));
        REQUIRE(strict->takeDiagnostics().isEmpty());
        const auto syntax = decoder(source, Syntax);
        const auto tokens = read(syntax);
        auto joined = el::StringEditor{};
        for (const auto &token : tokens) {
            joined.append(token.rawText());
        }
        REQUIRE_EQUAL(el::String{joined}, source);
        REQUIRE_EQUAL(syntax->takeDiagnostics().count(), el::ItemCount{1});
        REQUIRE(syntax->takeDiagnostics().isEmpty());
    }

    void testSyntaxModeNeverResolvesPlaceholders() {
        using enum el::conf::impl::TokenDecoderMode;
        auto provider = std::make_shared<CountingSource>();
        auto registry = std::make_shared<el::text::placeholder::impl::Registry>();
        registry->addSource(provider);
        const auto source = "x: \"${count:value}\"\ny: \"\"\"\n    ${count:value}\n    \"\"\"\n"_el;
        const auto syntax = decoder(source, Syntax, registry);
        const auto tokens = read(syntax);
        REQUIRE_EQUAL(provider->calls, 0);
        auto textCount = 0;
        for (const auto &token : tokens) {
            if (token.type() == el::conf::impl::TokenType::Text ||
                token.type() == el::conf::impl::TokenType::MultiLineText) {
                REQUIRE_EQUAL(std::get<el::String>(token.content()), "${count:value}"_el);
                ++textCount;
            }
        }
        REQUIRE_EQUAL(textCount, 2);
        [[maybe_unused]] const auto expanded = read(decoder(source, Parsing, registry));
        REQUIRE_EQUAL(provider->calls, 2);
    }

    void testInternalFailuresAreNotSourceDiagnostics() {
        const auto syntax = decoder("x: 1"_el, el::conf::impl::TokenDecoderMode::Syntax);
        const auto error = el::conf::ConfError{el::conf::ConfErrorCategory::Internal, "Injected failure."_el};
        REQUIRE_THROWS_AS(el::conf::ConfError, syntax->recover(error, el::LineIndex{}));
        REQUIRE(syntax->takeDiagnostics().isEmpty());
        REQUIRE_EQUAL(syntax->character(), U'x');
    }
};
