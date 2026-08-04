// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/impl/lexer/Lexer.hpp>
#include <erbsland/conf/impl/source/FileSource.hpp>
#include <erbsland/conf/Integer.hpp>
#include <erbsland/conf/Source.hpp>

#include <sstream>
#include <utility>

using namespace el::conf;
using impl::CharStream;
using impl::CharStreamPtr;
using impl::FileSource;
using impl::Lexer;
using impl::LexerPtr;
using impl::LexerToken;
using impl::TokenGenerator;
using impl::TokenType;

/// Provide fixtures and assertions for lexer tests.
/// @notest{Shared unit-test helper.}
class LexerTestHelper : public ConfTestHelper {
public:
    std::variant<el::text::String, std::filesystem::path> testContentSource;
    SourcePtr source;
    CharStreamPtr decoder;
    LexerPtr lexer;
    LexerToken token;
    TokenGenerator tokenGenerator;

    auto additionalErrorMessages() -> std::string override {
        try {
            std::ostringstream oss;
            oss << "Tested content:\n" << el::text::StringConverter{testContents}.toStdString() << "\n";
            if (lexer != nullptr) {
                oss << "Lexer state:\n";
                oss << el::text::StringConverter{internalView(*lexer)->toString(2)}.toStdString() << "\n";
            }
            oss << "Last lexer token:\n";
            oss << el::text::StringConverter{internalView(token)->toString(2)}.toStdString() << "\n";
            if (std::holds_alternative<std::filesystem::path>(testContentSource)) {
                oss << "Lexing last test file again:\n";
                source = FileSource::fromFile(el::path::Path{std::get<std::filesystem::path>(testContentSource)});
            } else {
                oss << "Lexing last test string again:\n";
                source = FileSource::fromString(std::get<el::text::String>(testContentSource));
            }
            REQUIRE_NOTHROW(source->open());
            decoder = CharStream::create(source);
            lexer = Lexer::create(decoder);
            try {
                auto index = 0;
                for (auto token : lexer->tokens()) {
                    oss << index << ":\n";
                    oss << el::text::StringConverter{internalView(token)->toString(2)}.toStdString() << "\n";
                    index += 1;
                }
            } catch (ConfError &e) {
                oss << el::text::StringConverter{e.category().toText()}.toStdString()
                    << " exception: " << el::text::StringConverter{e.description()}.toStdString() << "\n";
            }
            return oss.str();
        } catch (...) {
            return "Unexpected exception.";
        }
    }

    void tearDown() override {
        // free all resources here to avoid side effects from deconstruction in the next test.
        tokenGenerator = {};
        lexer.reset();
        decoder.reset();
        source.reset();
        testContentSource = {};
        cleanUpTestFileDirectory();
    }

    /// Set up a lexer for text content.
    void setupLexer(const el::text::String &content) {
        testContentSource = content;
        source = createTestMemorySource(content);
        REQUIRE(source != nullptr);
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        REQUIRE(decoder != nullptr);
        lexer = Lexer::create(decoder);
        REQUIRE(lexer != nullptr);
    }

    /// Set up a lexer for byte content stored in a file.
    void setupLexer(const el::mem::ByteBlock &content) {
        auto path = createTestFile(content);
        testContentSource = path;
        source = Source::fromFile(el::path::Path{path});
        REQUIRE(source != nullptr);
        REQUIRE_NOTHROW(source->open());
        decoder = CharStream::create(source);
        REQUIRE(decoder != nullptr);
        lexer = Lexer::create(decoder);
        REQUIRE(lexer != nullptr);
    }

    /// Set up a token generator for the given content.
    template <typename T>
    void setupTokenGenerator(const T &content) {
        setupLexer(content);
        tokenGenerator = lexer->tokens();
    }

    /// Set up a token generator without diagnostic test content.
    void setupTokenGeneratorFast(const el::text::String &content) {
        testContentSource = {};
        testContents = {};
        source = Source::fromString(content);
        source->open();
        decoder = CharStream::create(source);
        lexer = Lexer::create(decoder);
        tokenGenerator = lexer->tokens();
    }

    /// Read the next token into the current token field.
    void readNextToken() {
        auto nextToken = std::optional<LexerToken>{};
        REQUIRE_NOTHROW(nextToken = tokenGenerator.next());
        REQUIRE(nextToken.has_value());
        token = std::move(*nextToken);
    }

    /// Require the next token to have the expected metadata.
    void requireNextToken(
        TokenType expectedTokenType,
        const std::optional<el::text::String> &expectedRaw = std::nullopt,
        std::optional<el::unit::CodeLocation> expectedBegin = std::nullopt,
        std::optional<el::unit::CodeLocation> expectedEnd = std::nullopt) {

        readNextToken();
        REQUIRE_EQUAL(token.type(), expectedTokenType);
        if (expectedRaw.has_value()) {
            REQUIRE_EQUAL(token.rawText(), *expectedRaw);
        } else {
            REQUIRE_FALSE(token.rawText().isEmpty())
        }
        if (expectedBegin.has_value()) {
            REQUIRE(token.begin() == expectedBegin);
        } else {
            REQUIRE_FALSE(token.begin().isUndefined());
        }
        if (expectedEnd.has_value()) {
            REQUIRE(token.end() == expectedEnd);
        } else {
            REQUIRE_FALSE(token.end().isUndefined());
        }
    }

    /// Require the next token to have the expected typed value.
    template <typename T>
    void requireNextValueToken(
        const TokenType expectedTokenType, const T &expectedValue, const std::optional<el::text::String> &expectedRaw) {

        readNextToken();
        REQUIRE_EQUAL(token.type(), expectedTokenType);
        REQUIRE(std::holds_alternative<T>(token.content()));
        const auto actualValue = std::get<T>(token.content());
        REQUIRE_EQUAL(actualValue, expectedValue);
        if (expectedRaw.has_value()) {
            REQUIRE_EQUAL(token.rawText(), *expectedRaw);
        } else {
            REQUIRE_FALSE(token.rawText().isEmpty())
        }
    }

    /// Require the next token to contain the expected string.
    void requireNextStringToken(
        const TokenType expectedTokenType,
        const el::text::String &expectedString,
        const std::optional<el::text::String> &expectedRaw = std::nullopt) {

        requireNextValueToken<el::text::String>(expectedTokenType, expectedString, expectedRaw);
    }

    /// Require the next token to contain the expected integer.
    void requireNextIntegerToken(
        const TokenType expectedTokenType,
        const Integer expectedValue,
        const std::optional<el::text::String> &expectedRaw = std::nullopt) {

        requireNextValueToken<Integer>(expectedTokenType, expectedValue, expectedRaw);
    }

    /// Require the next token to contain the expected bytes.
    void requireNextBytesToken(
        const TokenType expectedTokenType,
        const el::mem::ByteBlock &expectedValue,
        const std::optional<el::text::String> &expectedRaw = std::nullopt) {

        requireNextValueToken<el::mem::ByteBlock>(expectedTokenType, expectedValue, expectedRaw);
    }

    /// Require the next token read to raise the expected error.
    void requireError(
        ConfErrorCategory expectedErrorCategory,
        const std::optional<el::unit::CodeLocation> &expectedPosition = std::nullopt) {
        try {
            static_cast<void>(tokenGenerator.next());
            REQUIRE(false);
        } catch (const ConfError &e) {
            REQUIRE_EQUAL(e.category(), expectedErrorCategory);
            if (expectedPosition.has_value()) {
                REQUIRE_EQUAL(e.location(), *expectedPosition);
            }
        }
    }

    /// Require the next token read to raise one of the expected errors.
    void requireError(std::initializer_list<ConfErrorCategory> expectedErrorCategories) {
        try {
            static_cast<void>(tokenGenerator.next());
            REQUIRE(false);
        } catch (const ConfError &e) {
            const auto it = std::ranges::find(expectedErrorCategories, e.category());
            REQUIRE(it != expectedErrorCategories.end());
        }
    }

    /// Require that token generation has reached end of data.
    void requireEndOfData() {
        readNextToken();
        REQUIRE(token.type() == TokenType::EndOfData);
        REQUIRE(token.rawText().isEmpty());
        REQUIRE(token.begin().isUndefined());
        REQUIRE(token.end().isUndefined());
        REQUIRE_FALSE(tokenGenerator.next().has_value());
    }
};

/// Require the next lexer token to contain the expected floating-point value.
template <>
inline void LexerTestHelper::requireNextValueToken<Float>(
    const TokenType expectedTokenType, const Float &expectedValue, const std::optional<el::text::String> &expectedRaw) {

    readNextToken();
    REQUIRE_EQUAL(token.type(), expectedTokenType);
    REQUIRE(std::holds_alternative<double>(token.content()));
    const auto actualValue = std::get<double>(token.content());
    if (std::isnan(expectedValue)) {
        REQUIRE(std::isnan(actualValue));
    } else if (std::isinf(expectedValue)) {
        REQUIRE(std::isinf(actualValue));
    } else {
        REQUIRE(std::abs(actualValue - expectedValue) < std::numeric_limits<Float>::epsilon());
    }
    if (expectedRaw.has_value()) {
        REQUIRE_EQUAL(token.rawText(), *expectedRaw);
    } else {
        REQUIRE_FALSE(token.rawText().isEmpty())
    }
}
