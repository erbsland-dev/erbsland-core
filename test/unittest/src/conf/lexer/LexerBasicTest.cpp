// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
class LexerBasicTest final : public UNITTEST_SUBCLASS(LexerTestHelper) {
public:
    void testZeroFile() {
        setupTokenGenerator(el::text::String{}); // zero length file.
        WITH_CONTEXT(requireEndOfData());
    }

    void testJustSpacing() {
        setupTokenGenerator("    "_el);
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireEndOfData());
    }

    void testJustSpacingWithLineBreaks() {
        setupTokenGenerator("    \n    \n    "_el);
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireEndOfData());
        setupTokenGenerator("    \r\n    \r\n    "_el);
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\r\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\r\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireEndOfData());
    }

    void testEmptyLinesWithComments() {
        setupTokenGenerator("    \n\n    # comment\n\n      # comment at end"_el);
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireNextToken(TokenType::Comment, "# comment"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "      "_el));
        WITH_CONTEXT(requireNextToken(TokenType::Comment, "# comment at end"_el));
        WITH_CONTEXT(requireEndOfData());
    }

    void testErrorPropagationSyntax() {
        // Unexpected character - syntax error from lexer.
        setupTokenGenerator("    x"_el);
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireError(
            ConfErrorCategory::Syntax,
            el::unit::CodeLocation{el::unit::LineIndex::zero(), el::unit::ColumnIndex{4U}, el::unit::CpIndex{4U}}));
    }

    void testErrorPropagationControlCharacter() {
        // Control character - exception from decoder.
        setupTokenGenerator("    \x01"_el);
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::Character));
    }

    void testErrorPropagationEncodingError() {
        // Invalid UTF-8 encoding - exception from decoder.
        // The error should happen after reading the spacing.
        auto invalidUtf8 = std::string(4, ' ');
        invalidUtf8.push_back(static_cast<char>(0x81U));
        invalidUtf8.push_back(static_cast<char>(0x82U));
        REQUIRE_NOTHROW(setupTokenGenerator(el::text::String{invalidUtf8}));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "    "_el));
        WITH_CONTEXT(requireError(ConfErrorCategory::Encoding));
    }

    void testAccessAfterRead() {
        setupLexer("    \n    \n"_el);
        // read all tokens.
        for (auto token : lexer->tokens()) {
            // ignore
        }
        try {
            for (auto token : lexer->tokens()) {
                REQUIRE(false); // This must not work.
            }
            REQUIRE(false);     // This must not work either.
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::Internal);
        }
    }

    void testSyntaxMetaName() {
        setupTokenGenerator("@version: \"1.0\"\n"_el);
        WITH_CONTEXT(requireNextStringToken(TokenType::MetaName, "@version"_el, "@version"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NameValueSeparator, ":"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::Text, "1.0"_el, "\"1.0\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("@signature: \"data\"\n[main]\n"_el);
        WITH_CONTEXT(requireNextStringToken(TokenType::MetaName, "@signature"_el, "@signature"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NameValueSeparator, ":"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::Text, "data"_el, "\"data\""_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextToken(TokenType::RegularName, "main"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());
    }

    void testComment() {
        setupTokenGenerator("# comment\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::Comment, "# comment"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());
    }

    void testSectionMap() {
        setupTokenGenerator("[section]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("----[section]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "----["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("[section]----\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]----"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("-[ . \t  relative . section.with.elements ]-   # and a comment\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "-["_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " \t  "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "relative"_el, "relative"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "with"_el, "with"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "elements"_el, "elements"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]-"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "   "_el));
        WITH_CONTEXT(requireNextToken(TokenType::Comment, "# and a comment"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("[section]*\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        // don't accept the trailing asterisk
        WITH_CONTEXT(requireError(ConfErrorCategory::Syntax));
    }

    void testSectionList() {
        setupTokenGenerator("*[section]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "*["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("*[section]*\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "*["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]*"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("----*[section]\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "----*["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("----*[section]*\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "----*["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]*"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("*[section]----\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "*["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]----"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("*[section]*----\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "*["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]*----"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());

        setupTokenGenerator("-*[ . \t  relative . section.with.elements ]*-   # and a comment\n"_el);
        WITH_CONTEXT(requireNextToken(TokenType::SectionListOpen, "-*["_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " \t  "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "relative"_el, "relative"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "with"_el, "with"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NamePathSeparator, "."_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "elements"_el, "elements"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionListClose, "]*-"_el));
        WITH_CONTEXT(requireNextToken(TokenType::Spacing, "   "_el));
        WITH_CONTEXT(requireNextToken(TokenType::Comment, "# and a comment"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireEndOfData());
    }

    void testDocumentWithDigest() {
        // verify the used algorithm.
        REQUIRE_EQUAL(el::conf::impl::defaults::documentHashAlgorithm, el::cryptology::HashAlgorithm::Sha3_256);
        setupTokenGenerator("@signature: \"data\"\n[main]\nvalue: 123\nanother value: \"example\"\n"_el);
        while (auto nextToken = tokenGenerator.next()) {
            token = std::move(*nextToken);
        }
        REQUIRE_EQUAL(
            lexer->digest(), bytesFromHex("b352bf8f49d930ec1267659eddaee1a1a6f38840e7d67ef5733ca2cee83f6633"_el));
    }
};
