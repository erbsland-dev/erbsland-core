// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/impl/NamedKeyEntry.hpp>
#include <erbsland/text/impl/NamedKeyFormat.hpp>
#include <erbsland/text/impl/NamedKeyParser.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringCharReader.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/unit/CpIndex.hpp>
#include <erbsland/unit/CpLength.hpp>
#include <erbsland/unit/ItemCount.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/Set.hpp>

using namespace el::text;
using namespace el::text::literals;

TESTED_TARGETS(NamedKeyEntryKind NamedKeyEntry NamedKeyFormat NamedKeyParser)
class NamedKeyParserTest final : public el::UnitTest {
    using NamedKeyEntry = el::text::impl::NamedKeyEntry;
    using NamedKeyEntryKind = el::text::impl::NamedKeyEntryKind;
    using NamedKeyFormat = el::text::impl::NamedKeyFormat;
    using NamedKeyParser = el::text::impl::NamedKeyParser;

private:
    enum Key : int {
        Alpha = 1,
        Beta = 2,
        Width = 3,
    };

private:
    [[nodiscard]] static auto standardFormat() -> NamedKeyFormat {
        return NamedKeyFormat{}
            .setKeys({
                {"alpha"_el, Alpha},
                {"a"_el, Alpha},
                {"beta"_el, Beta},
                {"width"_el, Width},
                {"w"_el, Width},
            })
            .setAllowedKeyPrefixes(CharSet{U'+', U'-'})
            .setValueWithoutKeySeparatorChars(CharSet::from(AsciiCategory::Digit));
    }

    template <typename Function>
    void requirePositionedParseError(Function function, const el::unit::CpIndex position) {
        try {
            function();
            REQUIRE(false);
        } catch (const el::err::ParseError &error) {
            REQUIRE(error.hasPosition());
            REQUIRE_EQUAL(error.codePointIndex(), position);
        }
    }

public:
    void testKeyTableNormalizationAndCanonicalName() {
        const auto keys = NamedKeyFormat::Keys{{
            {"alpha_key"_el, Alpha},
            {"alias"_el, Alpha},
        }};
        auto format = NamedKeyFormat{}.setKeys(keys);

        REQUIRE_EQUAL(format.keyIndex("ALPHA_KEY"_el).value(), Alpha);
        REQUIRE_EQUAL(format.keyIndex("alpha key"_el).value(), Alpha);
        REQUIRE_FALSE(format.keyIndex("unknown"_el).has_value());
        REQUIRE_EQUAL(format.keyName(Alpha), "alpha_key"_el);
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(format.keyName(Beta)));
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(NamedKeyFormat{}.setKeys({{"Alpha Key"_el, Alpha}})));
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(NamedKeyFormat{}.addKey("ALPHA"_el, Alpha)));
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(NamedKeyFormat{}.addKey("alpha"_el, -1)));
        REQUIRE_THROWS_AS(
            el::err::LogicError, static_cast<void>(NamedKeyFormat{}.setKeys({{"same"_el, Alpha}, {"same"_el, Beta}})));

        REQUIRE_THROWS_AS(
            el::err::LogicError, static_cast<void>(format.setKeys({{"duplicate"_el, Alpha}, {"duplicate"_el, Beta}})));
        REQUIRE_EQUAL(format.keyName(Alpha), "alpha_key"_el);

        auto incremental = NamedKeyFormat{};
        incremental.addKey("alpha"_el, Alpha);
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(incremental.addKey("alpha"_el, Beta)));
    }

    void testKeyFormsPrefixesAndRepeatedEnd() {
        auto format = standardFormat().setStopCharacter(U';');
        auto reader = StringCharReader{String{"+ALPHA,beta=value,w32;tail"_el}};
        auto parser = NamedKeyParser{reader, format};

        const auto alpha = parser.readEntry();
        REQUIRE(alpha.isKey());
        REQUIRE_EQUAL(alpha.prefix(), U'+');
        REQUIRE_EQUAL(alpha.keyIndex(), Alpha);
        REQUIRE(alpha.value().isEmpty());

        const auto beta = parser.readEntry();
        REQUIRE(beta.isKeyWithValue());
        REQUIRE(beta.prefix().isNoCodePoint());
        REQUIRE_EQUAL(beta.keyIndex(), Beta);
        REQUIRE_EQUAL(beta.value(), "value"_el);

        const auto width = parser.readEntry();
        REQUIRE(width.isKeyWithValue());
        REQUIRE_EQUAL(width.keyIndex(), Width);
        REQUIRE_EQUAL(width.value(), "32"_el);

        REQUIRE(parser.readEntry().isEnd());
        REQUIRE(parser.readEntry().isEnd());
        REQUIRE_EQUAL(reader.peek(), U't');
        REQUIRE_EQUAL(parser.format().keyName(Width), "width"_el);
    }

    void testBulkReadAcrossStringWidths() {
        auto format = standardFormat();

        auto reader8 = StringCharReader{String{"alpha,beta=é,w12"_el}};
        const auto entries8 = NamedKeyParser{reader8, format}.readAllEntries();
        REQUIRE_EQUAL(entries8.count(), el::unit::ItemCount{3U});
        REQUIRE_EQUAL(entries8.get(el::unit::ItemIndex{1U}).value(), "é"_el);

        auto reader16 = StringCharReader{U16String{u"alpha,beta=é,w12"_el}};
        const auto entries16 = NamedKeyParser{reader16, format}.readAllEntries();
        REQUIRE_EQUAL(entries16.count(), el::unit::ItemCount{3U});
        REQUIRE_EQUAL(entries16.get(el::unit::ItemIndex{1U}).value(), "é"_el);

        auto reader32 = StringCharReader{U32String{U"alpha,beta=é,w12"_el}};
        const auto entries32 = NamedKeyParser{reader32, format}.readAllEntries();
        REQUIRE_EQUAL(entries32.count(), el::unit::ItemCount{3U});
        REQUIRE_EQUAL(entries32.get(el::unit::ItemIndex{1U}).value(), "é"_el);
    }

    void testAllowedKeysAndDuplicateAliases() {
        auto format = standardFormat();
        auto allowedReader = StringCharReader{String{"alpha,w3"_el}};
        auto allowedParser = NamedKeyParser{allowedReader, format};
        allowedParser.setAllowedKeys(el::util::Set<int>{Alpha, Width});
        REQUIRE_EQUAL(allowedParser.readAllEntries().count(), el::unit::ItemCount{2U});

        auto disallowedReader = StringCharReader{String{"beta=value"_el}};
        auto disallowedParser = NamedKeyParser{disallowedReader, format};
        disallowedParser.setAllowedKeys(el::util::Set<int>{Alpha});
        REQUIRE_THROWS_AS(el::err::ParseError, static_cast<void>(disallowedParser.readEntry()));

        auto noneReader = StringCharReader{String{"alpha"_el}};
        auto noneParser = NamedKeyParser{noneReader, format};
        noneParser.setAllowedKeys(el::util::Set<int>{});
        REQUIRE_THROWS_AS(el::err::ParseError, static_cast<void>(noneParser.readEntry()));

        auto duplicateReader = StringCharReader{String{"alpha,-a"_el}};
        REQUIRE_THROWS_AS(
            el::err::ParseError, static_cast<void>(NamedKeyParser{duplicateReader, format}.readAllEntries()));

        format.setUniqueKeysRequired(false);
        auto repeatedReader = StringCharReader{String{"alpha,-a"_el}};
        auto repeatedParser = NamedKeyParser{repeatedReader, format};
        REQUIRE_EQUAL(repeatedParser.readAllEntries().count(), el::unit::ItemCount{2U});
    }

    void testPositionalValueModeLocking() {
        auto format = standardFormat();
        auto reader = StringCharReader{String{"first,alpha,beta=value"_el}};
        const auto entries = NamedKeyParser{reader, format}.readAllEntries();

        REQUIRE_EQUAL(entries.count(), el::unit::ItemCount{3U});
        REQUIRE(entries.get(el::unit::ItemIndex{0U}).isValue());
        REQUIRE_EQUAL(entries.get(el::unit::ItemIndex{1U}).value(), "alpha"_el);
        REQUIRE_EQUAL(entries.get(el::unit::ItemIndex{2U}).value(), "beta=value"_el);

        auto mixedReader = StringCharReader{String{"alpha,unknown"_el}};
        REQUIRE_THROWS_AS(el::err::ParseError, static_cast<void>(NamedKeyParser{mixedReader, format}.readAllEntries()));

        auto explicitUnknownReader = StringCharReader{String{"unknown=value"_el}};
        REQUIRE_THROWS_AS(
            el::err::ParseError, static_cast<void>(NamedKeyParser{explicitUnknownReader, format}.readEntry()));
    }

    void testIndependentEntryFormPolicies() {
        auto noValues = standardFormat().setValuesAllowed(false).setValueListAllowed(true);
        auto keyValueReader = StringCharReader{String{"alpha=value"_el}};
        REQUIRE_THROWS_AS(el::err::ParseError, static_cast<void>(NamedKeyParser{keyValueReader, noValues}.readEntry()));
        auto positionalReader = StringCharReader{String{"free"_el}};
        REQUIRE(NamedKeyParser{positionalReader, noValues}.readEntry().isValue());

        auto valuesRequired = standardFormat().setKeysWithoutValuesAllowed(false);
        auto bareReader = StringCharReader{String{"alpha"_el}};
        REQUIRE_THROWS_AS(
            el::err::ParseError, static_cast<void>(NamedKeyParser{bareReader, valuesRequired}.readEntry()));
        auto valuedReader = StringCharReader{String{"alpha=value"_el}};
        REQUIRE(NamedKeyParser{valuedReader, valuesRequired}.readEntry().isKeyWithValue());

        auto noValueList = standardFormat().setValueListAllowed(false);
        auto unknownReader = StringCharReader{String{"free"_el}};
        REQUIRE_THROWS_AS(
            el::err::ParseError, static_cast<void>(NamedKeyParser{unknownReader, noValueList}.readEntry()));
    }

    void testValueLimitsAndAllowedCharacters() {
        auto format = standardFormat()
                          .setMaximumValues(el::unit::ItemCount{2U})
                          .setMaximumValueLength(el::unit::CpLength{3U})
                          .setAllowedValueChars(CharSet::fromPattern("a-z0-9="_el));
        auto validReader = StringCharReader{String{"foo,bar"_el}};
        auto validParser = NamedKeyParser{validReader, format};
        REQUIRE_EQUAL(validParser.readAllEntries().count(), el::unit::ItemCount{2U});

        auto countReader = StringCharReader{String{"a,b,c"_el}};
        REQUIRE_THROWS_AS(el::err::ParseError, static_cast<void>(NamedKeyParser{countReader, format}.readAllEntries()));

        auto lengthReader = StringCharReader{String{"abcd"_el}};
        REQUIRE_THROWS_AS(el::err::ParseError, static_cast<void>(NamedKeyParser{lengthReader, format}.readEntry()));

        auto characterReader = StringCharReader{String{"a!"_el}};
        REQUIRE_THROWS_AS(el::err::ParseError, static_cast<void>(NamedKeyParser{characterReader, format}.readEntry()));
    }

    void testSeparatorsStopsAndPositionedErrors() {
        auto format = standardFormat().setStopCharacter(U']');

        auto emptyReader = StringCharReader{String{",alpha]"_el}};
        WITH_CONTEXT(requirePositionedParseError(
            [&]() { static_cast<void>(NamedKeyParser{emptyReader, format}.readEntry()); }, el::unit::CpIndex{0U}));

        auto trailingReader = StringCharReader{String{"alpha,]"_el}};
        REQUIRE_THROWS_AS(
            el::err::ParseError, static_cast<void>(NamedKeyParser{trailingReader, format}.readAllEntries()));

        auto missingStopReader = StringCharReader{String{"alpha"_el}};
        REQUIRE_THROWS_AS(
            el::err::ParseError, static_cast<void>(NamedKeyParser{missingStopReader, format}.readAllEntries()));

        auto missingValueReader = StringCharReader{String{"alpha=]"_el}};
        WITH_CONTEXT(requirePositionedParseError(
            [&]() { static_cast<void>(NamedKeyParser{missingValueReader, format}.readEntry()); },
            el::unit::CpIndex{0U}));

        auto unsafeReader = StringCharReader{String{"alpha=\n]"_el}};
        REQUIRE_THROWS_AS(el::err::ParseError, static_cast<void>(NamedKeyParser{unsafeReader, format}.readEntry()));
    }

    void testInvalidFormatConfiguration() {
        auto sameSeparators = standardFormat().setValueSeparator(U',');
        auto reader = StringCharReader{String{"alpha"_el}};
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(NamedKeyParser{reader, sameSeparators}));

        auto conflictingCompact = standardFormat().setValueWithoutKeySeparatorChars(CharSet{U'a'});
        REQUIRE_THROWS_AS(el::err::LogicError, static_cast<void>(NamedKeyParser{reader, conflictingCompact}));
    }
};
