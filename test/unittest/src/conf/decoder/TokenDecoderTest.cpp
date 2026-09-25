// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/impl/char/NamedChars.hpp>
#include <erbsland/conf/impl/decoder/TokenDecoder.hpp>
#include <erbsland/conf/impl/source/FileSource.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/text/placeholder/EnvironmentSource.hpp>
#include <erbsland/text/placeholder/impl/Registry.hpp>

#include <sstream>

using namespace el::conf;
using el::conf::impl::CharClass;
using el::conf::impl::CharStream;
using el::conf::impl::FileSource;
using el::conf::impl::internalView;
using el::conf::impl::LexerToken;
using el::conf::impl::TokenDecoder;
using el::conf::impl::TokenDecoderPtr;
using el::conf::impl::TokenType;
using el::text::placeholder::EnvironmentSource;
using el::text::placeholder::impl::Registry;
using namespace el::text::literals;
namespace nc = el::conf::impl::nc;

TESTED_TARGETS(TokenDecoder)
class TokenDecoderTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    std::filesystem::path testFile;
    SourcePtr source;
    TokenDecoderPtr decoder;

    void requireLocation(const std::size_t line, const std::size_t column) {
        const auto location = decoder->location().codeLocation();
        REQUIRE_EQUAL(location.line(), el::unit::LineIndex::fromSizeT(line));
        REQUIRE_EQUAL(location.column(), el::unit::ColumnIndex::fromSizeT(column));
        REQUIRE_FALSE(location.position().isNoIndex());
    }

    void tearDown() override {
        // free all resources here to avoid side effects from deconstruction in the next test.
        decoder.reset();
        source.reset();
        testFile.clear();
        cleanUpTestFileDirectory();
    }

    auto additionalErrorMessages() -> std::string override {
        try {
            std::ostringstream oss;
            oss << "Test file path: " << testFile.string() << "\n";
            oss << "Source: " << el::text::StringConverter{source->identifier()->toText()}.toStdString() << "\n";
            oss << "Buffered Token Decoder State:\n"
                << el::text::StringConverter{internalView(*decoder)->toString(2)}.toStdString();
            return oss.str();
        } catch (...) {
            return "Unexpected exception.";
        }
    }

    template <typename T>
    void setupDecoder(const T &content) {
        testFile = createTestFile(content);
        REQUIRE(!testFile.empty());
        source = FileSource::fromFile(el::path::Path{testFile});
        REQUIRE_NOTHROW(source->open());
        REQUIRE(source);
        decoder = TokenDecoder::create(CharStream::create(source));
        REQUIRE(decoder);
        decoder->initialize();
    }

    void setupStringDecoder(el::text::String content) {
        source = Source::fromString(std::move(content));
        REQUIRE_NOTHROW(source->open());
        decoder = TokenDecoder::create(CharStream::create(source));
        REQUIRE(decoder);
        decoder->initialize();
    }

    void requireAndNext(char32_t expectedUnicode) {
        REQUIRE_EQUAL(decoder->character(), expectedUnicode);
        decoder->next();
    }

    void requireEndOfLine() {
        REQUIRE_EQUAL(decoder->character(), CharClass::LineBreak);
        auto token = decoder->createEndOfLineToken();
        REQUIRE_EQUAL(token.type(), TokenType::LineBreak);
        REQUIRE_EQUAL(token.rawText(), el::text::String{"\n"_el});
    }

    void requireEndOfData() {
        REQUIRE(decoder->character().isEndOfData());
        auto token = decoder->createEndOfDataToken();
        REQUIRE_EQUAL(token.type(), TokenType::EndOfData);
        REQUIRE(token.rawText().isEmpty());
        REQUIRE(token.begin().isUndefined());
        REQUIRE(token.end().isUndefined());
    }

    void testEmptyFile() {
        setupDecoder(el::text::String{});
        REQUIRE_EQUAL(decoder->location().sourceIdentifier(), source->identifier());
        WITH_CONTEXT(requireLocation(0U, 0U));
        REQUIRE(decoder->character().isEndOfData());
    }

    void testPlaceholderAvailability() {
        source = Source::fromString(""_el);
        REQUIRE_NOTHROW(source->open());
        auto placeholderResolver = std::make_shared<Registry>();
        decoder = TokenDecoder::create(CharStream::create(source), placeholderResolver);
        REQUIRE_FALSE(decoder->hasPlaceholders());
        placeholderResolver->addSource(std::make_shared<EnvironmentSource>());
        REQUIRE(decoder->hasPlaceholders());
    }

    void testSimpleSequence() {
        setupDecoder("abc\n😀\nxyz"_el);
        WITH_CONTEXT(requireLocation(0U, 0U));
        WITH_CONTEXT(requireAndNext(U'a'));
        WITH_CONTEXT(requireLocation(0U, 1U));
        WITH_CONTEXT(requireAndNext(U'b'));
        WITH_CONTEXT(requireLocation(0U, 2U));
        WITH_CONTEXT(requireAndNext(U'c'));
        WITH_CONTEXT(requireLocation(0U, 3U));
        auto token = decoder->createToken(TokenType::Text, ""_el);
        REQUIRE_EQUAL(token.rawText(), el::text::String{"abc"_el});
        WITH_CONTEXT(requireEndOfLine());
        WITH_CONTEXT(requireAndNext(U'😀'));
        WITH_CONTEXT(requireLocation(1U, 1U));
        token = decoder->createToken(TokenType::Text, ""_el);
        REQUIRE_EQUAL(token.rawText(), el::text::String{"😀"_el});
        WITH_CONTEXT(requireEndOfLine());
        WITH_CONTEXT(requireAndNext(U'x'));
        WITH_CONTEXT(requireLocation(2U, 1U));
        WITH_CONTEXT(requireAndNext(U'y'));
        WITH_CONTEXT(requireLocation(2U, 2U));
        WITH_CONTEXT(requireAndNext(U'z'));
        WITH_CONTEXT(requireLocation(2U, 3U));
        token = decoder->createToken(TokenType::Text, ""_el);
        REQUIRE_EQUAL(token.rawText(), el::text::String{"xyz"_el});
        WITH_CONTEXT(requireEndOfData());
    }

    void testTransactionFromStart() {
        setupDecoder("abcdef"_el);
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'a'));
            WITH_CONTEXT(requireAndNext(U'b'));
            WITH_CONTEXT(requireAndNext(U'c'));
        } // rollback
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'a'));
            WITH_CONTEXT(requireAndNext(U'b'));
        } // rollback
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'a'));
            WITH_CONTEXT(requireAndNext(U'b'));
            WITH_CONTEXT(requireAndNext(U'c'));
            WITH_CONTEXT(requireAndNext(U'd'));
        } // rollback
        WITH_CONTEXT(requireAndNext(U'a'));
        WITH_CONTEXT(requireAndNext(U'b'));
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'c'));
            WITH_CONTEXT(requireAndNext(U'd'));
            WITH_CONTEXT(requireAndNext(U'e'));
            WITH_CONTEXT(requireAndNext(U'f'));
        } // rollback
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'c'));
            WITH_CONTEXT(requireAndNext(U'd'));
            transaction.commit();
        } // no rollback
        WITH_CONTEXT(requireAndNext(U'e'));
        WITH_CONTEXT(requireAndNext(U'f'));
        WITH_CONTEXT(requireEndOfData());
    }

    void testMultibyteCaptureAndEndRollback() {
        setupDecoder(u8"A😀β"_el);
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'A'));
            WITH_CONTEXT(requireAndNext(U'😀'));
            REQUIRE_EQUAL(transaction.capturedString(), u8"A😀"_el);
            REQUIRE_EQUAL(transaction.capturedSize(), std::size_t{2U});
            WITH_CONTEXT(requireAndNext(U'β'));
            REQUIRE(decoder->character().isEndOfData());
            REQUIRE_EQUAL(transaction.capturedString(), u8"A😀β"_el);
            REQUIRE_EQUAL(transaction.capturedSize(), std::size_t{3U});
        }
        WITH_CONTEXT(requireLocation(0U, 0U));
        WITH_CONTEXT(requireAndNext(U'A'));
        WITH_CONTEXT(requireAndNext(U'😀'));
        WITH_CONTEXT(requireAndNext(U'β'));
        WITH_CONTEXT(requireEndOfData());
    }

    void testRollbackAtLineBreak() {
        setupDecoder("abc\nx"_el);
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'a'));
            WITH_CONTEXT(requireAndNext(U'b'));
            WITH_CONTEXT(requireAndNext(U'c'));
            REQUIRE_EQUAL(decoder->character(), CharClass::LineBreak);
            REQUIRE_EQUAL(transaction.capturedString(), "abc"_el);
        }
        WITH_CONTEXT(requireLocation(0U, 0U));
        WITH_CONTEXT(requireAndNext(U'a'));
        WITH_CONTEXT(requireAndNext(U'b'));
        WITH_CONTEXT(requireAndNext(U'c'));
        const auto token = decoder->createToken(TokenType::Text);
        REQUIRE_EQUAL(token.rawText(), "abc"_el);
        WITH_CONTEXT(requireEndOfLine());
        WITH_CONTEXT(requireAndNext(U'x'));
        WITH_CONTEXT(requireEndOfData());
    }

    void testRollbackReplaysDelayedEncodingError() {
        auto content = std::string{"A"};
        content.push_back(static_cast<char>(0x81U));
        content.push_back('B');
        setupStringDecoder(el::text::String{std::string_view{content}});

        auto firstLocation = el::unit::CodeLocation{};
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'A'));
            REQUIRE(decoder->character().isError());
            try {
                decoder->checkForErrorAndThrowIt();
                REQUIRE(false);
            } catch (const ConfError &error) {
                REQUIRE_EQUAL(error.category(), ConfErrorCategory::Encoding);
                firstLocation = error.location();
            }
        }
        WITH_CONTEXT(requireAndNext(U'A'));
        REQUIRE(decoder->character().isError());
        try {
            decoder->checkForErrorAndThrowIt();
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::Encoding);
            REQUIRE_EQUAL(error.location(), firstLocation);
        }
    }

    void testRollbackReplaysDelayedCharacterError() {
        auto content = el::text::StringEditor{"A"_el};
        content.append(el::text::Char{U'\x01'});
        content.append(U'B');
        setupDecoder(el::text::String{content});

        auto firstLocation = el::unit::CodeLocation{};
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'A'));
            REQUIRE(decoder->character().isError());
            try {
                decoder->checkForErrorAndThrowIt();
                REQUIRE(false);
            } catch (const ConfError &error) {
                REQUIRE_EQUAL(error.category(), ConfErrorCategory::Character);
                firstLocation = error.location();
            }
        }
        WITH_CONTEXT(requireAndNext(U'A'));
        REQUIRE(decoder->character().isError());
        try {
            decoder->checkForErrorAndThrowIt();
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::Character);
            REQUIRE_EQUAL(error.location(), firstLocation);
        }
    }

    void testNestedTransactions1() {
        setupDecoder("abcdef"_el);
        {
            auto transaction1 = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'a'));
            WITH_CONTEXT(requireAndNext(U'b'));
            REQUIRE_EQUAL(transaction1.capturedString(), "ab"_el);
            {
                auto transaction2 = el::conf::impl::Transaction{*decoder};
                WITH_CONTEXT(requireAndNext(U'c'));
                WITH_CONTEXT(requireAndNext(U'd'));
                REQUIRE_EQUAL(transaction2.capturedString(), "cd"_el);
                {
                    auto transaction3 = el::conf::impl::Transaction{*decoder};
                    WITH_CONTEXT(requireAndNext(U'e'));
                    WITH_CONTEXT(requireAndNext(U'f'));
                    REQUIRE_EQUAL(transaction3.capturedString(), "ef"_el);
                    transaction3.commit();
                }
                REQUIRE_EQUAL(transaction2.capturedString(), "cdef"_el);
                transaction2.commit();
            }
            REQUIRE_EQUAL(transaction1.capturedString(), "abcdef"_el);
            transaction1.commit();
        }
        WITH_CONTEXT(requireEndOfData());
    }

    void testNestedTransactions2() {
        setupDecoder("abcdef"_el);
        {
            auto transaction1 = el::conf::impl::Transaction{*decoder};
            WITH_CONTEXT(requireAndNext(U'a'));
            WITH_CONTEXT(requireAndNext(U'b'));
            REQUIRE_EQUAL(transaction1.capturedString(), "ab"_el);
            {
                auto transaction2 = el::conf::impl::Transaction{*decoder};
                WITH_CONTEXT(requireAndNext(U'c'));
                WITH_CONTEXT(requireAndNext(U'd'));
                REQUIRE_EQUAL(transaction2.capturedString(), "cd"_el);
                {
                    auto transaction3 = el::conf::impl::Transaction{*decoder};
                    WITH_CONTEXT(requireAndNext(U'e'));
                    WITH_CONTEXT(requireAndNext(U'f'));
                    REQUIRE_EQUAL(transaction3.capturedString(), "ef"_el);
                    transaction3.commit();
                }
                REQUIRE_EQUAL(transaction2.capturedString(), "cdef"_el);
                // ROLLBACK
            }
            WITH_CONTEXT(requireAndNext(U'c'));
            WITH_CONTEXT(requireAndNext(U'd'));
            {
                auto transaction2 = el::conf::impl::Transaction{*decoder};
                WITH_CONTEXT(requireAndNext(U'e'));
                WITH_CONTEXT(requireAndNext(U'f'));
                REQUIRE_EQUAL(transaction2.capturedString(), "ef"_el);
                // ROLLBACK
            }
            WITH_CONTEXT(requireAndNext(U'e'));
            WITH_CONTEXT(requireAndNext(U'f'));
            REQUIRE_EQUAL(transaction1.capturedString(), "abcdef"_el);
            transaction1.commit();
        }
        WITH_CONTEXT(requireEndOfData());
    }

    void testDocumentWithDigest() {
        // verify the used algorithm.
        REQUIRE_EQUAL(el::conf::impl::defaults::documentHashAlgorithm, el::cryptology::HashAlgorithm::Sha3_256);
        setupDecoder("@signature: \"...\"\n[main]\nvalue: 123\nanother value: \"example\"\n"_el);
        while (!decoder->character().isEndOfData()) {
            decoder->next();
        }
        REQUIRE_EQUAL(
            decoder->digest(), bytesFromHex("b352bf8f49d930ec1267659eddaee1a1a6f38840e7d67ef5733ca2cee83f6633"_el));
    }

    void testDigestStableAfterSpeculativeEndOfData() {
        setupDecoder("@signature: \"...\"\n[main]\nvalue: 123"_el);
        while (decoder->character() != U'v') {
            decoder->next();
        }
        auto speculativeDigest = el::mem::ByteBlock{};
        {
            auto transaction = el::conf::impl::Transaction{*decoder};
            while (!decoder->character().isEndOfData()) {
                decoder->next();
            }
            speculativeDigest = decoder->digest();
            REQUIRE_FALSE(speculativeDigest.isEmpty());
        }
        REQUIRE_EQUAL(decoder->character(), U'v');
        while (!decoder->character().isEndOfData()) {
            decoder->next();
        }
        REQUIRE_EQUAL(decoder->digest(), speculativeDigest);
    }
};
