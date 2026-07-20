// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LexerTestHelper.hpp"

class LexerValueTestHelper : public LexerTestHelper {
public:
    inline static const auto cSectionLine = el::text::String{"[section]\n"_el};
    inline static const auto cValueStart = el::text::String{"value:"_el};
    inline static const auto cValueOnSameLineSpacing = el::text::String{" "_el};
    inline static const auto cValueOnNextLineSpacing = el::text::String{"\n"_el};
    inline static const auto cSimpleComment = el::text::String{"# comment"_el};
    inline static const auto cValueOnNextLineIndentationPattern1 = el::text::String{"    "_el};
    inline static const auto cValueOnNextLineIndentationPattern2 = el::text::String{"\t"_el};
    inline static const auto cValueOnNextLineIndentationPattern3 = el::text::String{" \t \t"_el};
    inline static const auto cFastPrefix = el::text::String{"[section]\nvalue: "_el};
    inline static const auto cFastSuffix = el::text::String{"\n"_el};

    enum class PrefixFormat : uint8_t {
        SameLine,
        NextLinePattern1,
        NextLinePattern2,
        NextLinePattern3,
        NextLinePattern1withComment,
    };

    static auto to_string(const PrefixFormat prefixFormat) -> std::string {
        switch (prefixFormat) {
        case PrefixFormat::SameLine:
            return "SameLine";
        case PrefixFormat::NextLinePattern1:
            return "NextLinePattern1";
        case PrefixFormat::NextLinePattern2:
            return "NextLinePattern2";
        case PrefixFormat::NextLinePattern3:
            return "NextLinePattern3";
        case PrefixFormat::NextLinePattern1withComment:
            return "NextLinePattern1withComment";
        }
        return {};
    }

    static constexpr auto cPrefixFormats = std::array<PrefixFormat, 5>{
        PrefixFormat::SameLine,
        PrefixFormat::NextLinePattern1,
        PrefixFormat::NextLinePattern2,
        PrefixFormat::NextLinePattern3,
        PrefixFormat::NextLinePattern1withComment,
    };

    enum class SuffixFormat : uint8_t { EndOfDocument, LineBreak, Comment, SpaceAndComment };

    static auto to_string(const SuffixFormat suffixFormat) -> std::string {
        switch (suffixFormat) {
        case SuffixFormat::EndOfDocument:
            return "EndOfDocument";
        case SuffixFormat::LineBreak:
            return "LineBreak";
        case SuffixFormat::Comment:
            return "Comment";
        case SuffixFormat::SpaceAndComment:
            return "SpaceAndComment";
        }
        return {};
    }

    static constexpr auto cSuffixPatterns = std::array<SuffixFormat, 4>{
        SuffixFormat::EndOfDocument, SuffixFormat::LineBreak, SuffixFormat::Comment, SuffixFormat::SpaceAndComment};

    static auto join(std::initializer_list<el::text::String> parts) -> el::text::String {
        return el::text::StringEditor::fromJoined(parts);
    }

    static void documentPrefix(el::text::StringEditor &doc, PrefixFormat prefixFormat) {
        doc.reserve(el::unit::ByteLength{1024U});
        doc.append(cSectionLine);
        doc.append(cValueStart);
        switch (prefixFormat) {
        case PrefixFormat::SameLine:
            doc.append(cValueOnSameLineSpacing);
            break;
        case PrefixFormat::NextLinePattern1:
            doc.append(cValueOnNextLineSpacing);
            doc.append(cValueOnNextLineIndentationPattern1);
            break;
        case PrefixFormat::NextLinePattern2:
            doc.append(cValueOnNextLineSpacing);
            doc.append(cValueOnNextLineIndentationPattern2);
            break;
        case PrefixFormat::NextLinePattern3:
            doc.append(cValueOnNextLineSpacing);
            doc.append(cValueOnNextLineIndentationPattern3);
            break;
        case PrefixFormat::NextLinePattern1withComment:
            doc.append(" "_el);
            doc.append(cSimpleComment);
            doc.append(cValueOnNextLineSpacing);
            doc.append(cValueOnNextLineIndentationPattern1);
            break;
        default:
            throw std::logic_error("Invalid prefix format.");
        }
    }

    static void documentSuffix(el::text::StringEditor &doc, const SuffixFormat suffixFormat) {
        switch (suffixFormat) {
        case SuffixFormat::EndOfDocument:
            break;
        case SuffixFormat::LineBreak:
            doc.append("\n"_el);
            break;
        case SuffixFormat::Comment:
            doc.append(cSimpleComment);
            break;
        case SuffixFormat::SpaceAndComment:
            doc.append(" "_el);
            doc.append(cSimpleComment);
            break;
        default:
            throw std::logic_error("Invalid suffix format.");
        }
    }

    void setupTokenGeneratorForValueTest(
        const el::text::String &valueText, const PrefixFormat prefixFormat, const SuffixFormat suffixFormat) {
        el::text::StringEditor doc;
        doc.reserve(el::unit::ByteLength{250U});
        documentPrefix(doc, prefixFormat);
        doc.append(valueText);
        documentSuffix(doc, suffixFormat);
        setupTokenGenerator(el::text::String{doc});
    }

    void setupTokenGeneratorForMassValueTest(const el::text::String &valueText) {
        auto doc = el::text::StringEditor{cFastPrefix};
        doc.append(valueText);
        doc.append(cFastSuffix);
        setupTokenGeneratorFast(el::text::String{doc});
    }

    static auto indentForPrefix(PrefixFormat prefixFormat) -> el::text::String {
        switch (prefixFormat) {
        case PrefixFormat::NextLinePattern1:
        case PrefixFormat::NextLinePattern1withComment:
            return cValueOnNextLineIndentationPattern1;
        case PrefixFormat::NextLinePattern2:
            return cValueOnNextLineIndentationPattern2;
        case PrefixFormat::NextLinePattern3:
            return cValueOnNextLineIndentationPattern3;
        default:
            throw std::logic_error("Invalid indentation pattern.");
        }
    }

    void verifyPrefix(PrefixFormat prefixFormat) {
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapOpen, "["_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el));
        WITH_CONTEXT(requireNextToken(TokenType::SectionMapClose, "]"_el));
        WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
        WITH_CONTEXT(requireNextStringToken(TokenType::RegularName, "value"_el, "value"_el));
        WITH_CONTEXT(requireNextToken(TokenType::NameValueSeparator, ":"_el));
        switch (prefixFormat) {
        case PrefixFormat::SameLine:
            WITH_CONTEXT(requireNextToken(TokenType::Spacing, cValueOnSameLineSpacing));
            break;
        case PrefixFormat::NextLinePattern1:
        case PrefixFormat::NextLinePattern2:
        case PrefixFormat::NextLinePattern3:
            WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
            WITH_CONTEXT(requireNextToken(TokenType::Indentation, indentForPrefix(prefixFormat)));
            break;
        case PrefixFormat::NextLinePattern1withComment:
            WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
            WITH_CONTEXT(requireNextToken(TokenType::Comment, cSimpleComment));
            WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
            WITH_CONTEXT(requireNextToken(TokenType::Indentation, cValueOnNextLineIndentationPattern1));
            break;
        default:
            throw std::logic_error("Invalid indentation pattern.");
        }
    }

    void verifySameLinePrefixFast() {
        requireNextToken(TokenType::SectionMapOpen, "["_el);
        requireNextStringToken(TokenType::RegularName, "section"_el, "section"_el);
        requireNextToken(TokenType::SectionMapClose, "]"_el);
        requireNextToken(TokenType::LineBreak, "\n"_el);
        requireNextStringToken(TokenType::RegularName, "value"_el, "value"_el);
        requireNextToken(TokenType::NameValueSeparator, ":"_el);
        requireNextToken(TokenType::Spacing, cValueOnSameLineSpacing);
    }

    void verifySuffix(SuffixFormat suffixFormat) {
        switch (suffixFormat) {
        case SuffixFormat::EndOfDocument:
            WITH_CONTEXT(requireEndOfData());
            break;
        case SuffixFormat::LineBreak:
            WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
            WITH_CONTEXT(requireEndOfData());
            break;
        case SuffixFormat::Comment:
            WITH_CONTEXT(requireNextToken(TokenType::Comment, cSimpleComment));
            WITH_CONTEXT(requireEndOfData());
            break;
        case SuffixFormat::SpaceAndComment:
            WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
            WITH_CONTEXT(requireNextToken(TokenType::Comment, cSimpleComment));
            WITH_CONTEXT(requireEndOfData());
        }
    }

    void verifyNewLineSuffixFast() {
        requireNextToken(TokenType::LineBreak, "\n"_el);
        requireEndOfData();
    }

    template <typename T>
    void verifyValidValue(const el::text::String &valueText, TokenType tokenType, const T &expectedValue) {
        for (auto prefixFormat : cPrefixFormats) {
            for (auto suffixFormat : cSuffixPatterns) {
                setupTokenGeneratorForValueTest(valueText, prefixFormat, suffixFormat);
                WITH_CONTEXT(verifyPrefix(prefixFormat));
                WITH_CONTEXT(requireNextValueToken<T>(tokenType, expectedValue, valueText));
                WITH_CONTEXT(verifySuffix(suffixFormat));
            }
        }
    }

    template <typename T>
    void verifyValidValueFaster(const el::text::String &valueText, const TokenType tokenType, const T &expectedValue) {
        setupTokenGeneratorForMassValueTest(valueText);
        verifySameLinePrefixFast();
        requireNextValueToken<T>(tokenType, expectedValue, valueText);
        verifyNewLineSuffixFast();
    }

    void verifyValidInteger(const el::text::String &valueText, const Integer &expectedValue) {
        verifyValidValue<Integer>(valueText, TokenType::Integer, expectedValue);
    }

    void verifyValidBoolean(const el::text::String &valueText, const bool &expectedValue) {
        verifyValidValue<bool>(valueText, TokenType::Boolean, expectedValue);
    }

    void verifyValidText(const el::text::String &valueText, const el::text::String &expectedValue) {
        verifyValidValue<el::text::String>(valueText, TokenType::Text, expectedValue);
    }

    void verifyValidCode(const el::text::String &valueText, const el::text::String &expectedValue) {
        verifyValidValue<el::text::String>(valueText, TokenType::Code, expectedValue);
    }

    void verifyValidRegEx(const el::text::String &valueText, const el::text::String &expectedValue) {
        verifyValidValue<el::text::String>(valueText, TokenType::RegEx, expectedValue);
    }

    void verifyValidFloat(const el::text::String &valueText, const Float &expectedValue) {
        verifyValidValue<Float>(valueText, TokenType::Float, expectedValue);
    }

    void verifyValidTime(const el::text::String &valueText, const el::time::TimeWithZone &expectedValue) {
        verifyValidValue<el::time::TimeWithZone>(valueText, TokenType::Time, expectedValue);
    }

    void verifyValidTime(const el::text::String &valueText, const el::time::Time &expectedValue) {
        verifyValidValue<el::time::Time>(valueText, TokenType::Time, expectedValue);
    }

    void verifyValidDate(const el::text::String &valueText, const el::time::Date &expectedValue) {
        verifyValidValue<el::time::Date>(valueText, TokenType::Date, expectedValue);
    }

    void verifyValidDateTime(const el::text::String &valueText, const el::time::DateTime &expectedValue) {
        verifyValidValue<el::time::DateTime>(valueText, TokenType::DateTime, expectedValue);
    }

    void verifyValidTimeDelta(const el::text::String &valueText, const el::time::CalendarDelta &expectedValue) {
        verifyValidValue<el::time::CalendarDelta>(valueText, TokenType::TimeDelta, expectedValue);
    }

    void verifyValidByteData(const el::text::String &valueText, const el::mem::ByteBlock &expectedValue) {
        verifyValidValue<el::mem::ByteBlock>(valueText, TokenType::Bytes, expectedValue);
    }

    void verifyErrorInValue(const el::text::String &valueText, const ConfErrorCategory expectedError) {
        for (auto prefixFormat : cPrefixFormats) {
            for (auto suffixFormat : cSuffixPatterns) {
                setupTokenGeneratorForValueTest(valueText, prefixFormat, suffixFormat);
                WITH_CONTEXT(verifyPrefix(prefixFormat));
                WITH_CONTEXT(requireError(expectedError));
            }
        }
    }

    void verifyErrorInValue(
        const el::text::String &valueText, std::initializer_list<ConfErrorCategory> expectedErrors) {
        for (auto prefixFormat : cPrefixFormats) {
            for (auto suffixFormat : cSuffixPatterns) {
                setupTokenGeneratorForValueTest(valueText, prefixFormat, suffixFormat);
                WITH_CONTEXT(verifyPrefix(prefixFormat));
                WITH_CONTEXT(requireError(expectedErrors));
            }
        }
    }
};
