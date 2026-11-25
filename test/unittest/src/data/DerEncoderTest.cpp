// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Asn1TestHelper.hpp"

#include <erbsland/asn1/Asn1ObjectIdentifier.hpp>
#include <erbsland/ber/BerParser.hpp>
#include <erbsland/Date.hpp>
#include <erbsland/der/DerEncoder.hpp>
#include <erbsland/der/DerParser.hpp>
#include <erbsland/List.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/Time.hpp>
#include <erbsland/TimeAmounts.hpp>
#include <erbsland/Timestamp.hpp>

using namespace el::data::der;
using namespace el::text::literals;

TESTED_TARGETS(DerEncoder)
class DerEncoderTest final : public UNITTEST_SUBCLASS(Asn1TestHelper) {

public:
    void testPrimitiveValues() {
        auto encoder = el::data::der::DerEncoder{};
        encoder.appendBoolean(false);
        encoder.appendBoolean(true);
        encoder.appendPositiveInteger(0U);
        encoder.appendPositiveInteger(127U);
        encoder.appendPositiveInteger(128U);
        encoder.appendNull();
        encoder.appendOctetString(bytesFromHex("aabb").span());
        encoder.appendBitString(bytesFromHex("a0").span(), 4U);
        REQUIRE_EQUAL(encoder.encoded(), bytesFromHex("0101000101ff02010002017f0202008005000402aabb030204a0"));
    }

    void testObjectIdentifierAndCanonicalSet() {
        const auto oid = el::data::asn1::Asn1ObjectIdentifier::fromStringOrThrow("1.2.840.113549.1.1.11"_el);
        auto first = el::data::der::DerEncoder{};
        first.appendObjectIdentifier(oid);
        REQUIRE_EQUAL(first.encoded(), bytesFromHex("06092a864886f70d01010b"));
        auto one = el::data::der::DerEncoder{};
        one.appendPositiveInteger(1U);
        auto two = el::data::der::DerEncoder{};
        two.appendPositiveInteger(2U);
        const auto values = el::util::List<el::ByteBlock>{
            two.encoded(),
            one.encoded(),
        };
        auto set = el::data::der::DerEncoder{};
        set.appendSetOf(values);
        REQUIRE_EQUAL(set.encoded(), bytesFromHex("3106020101020102"));
    }

    void testTimeSelection() {
        using namespace el::time;

        const auto utc = Timestamp{Date::fromYearMonthDay(2049, 12, 31), Time{Hour{23}, Minute{59}, Second{58}}};
        const auto generalized = Timestamp{Date::fromYearMonthDay(2050, 1, 1), Time{Hour{0}, Minute{0}, Second{1}}};
        const auto generalizedWithOffset = Timestamp::fromDateTimeOrThrow(
            DateTime{Date::fromYearMonthDay(2050, 1, 1), Time{Hour{2}, Minute{0}, Second{1}}, Duration{Hours{2}}});
        auto encoder = el::data::der::DerEncoder{};
        encoder.appendTime(utc);
        encoder.appendTime(generalized);
        encoder.appendTime(generalizedWithOffset);
        REQUIRE_EQUAL(
            encoder.encoded(),
            bytesFromHex(
                "170d3439313233313233353935385a180f32303530303130313030303030315a"
                "180f32303530303130313030303030315a"));
    }

    void testNestedLengthBackpatching() {
        auto encoder = el::data::der::DerEncoder{};
        const auto sequence = encoder.beginSequence();
        encoder.appendOctetString(el::ByteBlock{el::ByteCount{128U}}.span());
        encoder.end(sequence);
        const auto expectedPrefix = bytesFromHex("308183048180");
        REQUIRE_EQUAL(encoder.encoded().length(), el::ByteCount{134U});
        REQUIRE_EQUAL(encoder.encoded().slice(el::ByteIndex::zero(), expectedPrefix.length()), expectedPrefix);
    }

    void testRejectedNonCanonicalValues() {
        auto encoder = el::data::der::DerEncoder{};
        REQUIRE_THROWS_AS(el::err::ParameterFailure, encoder.appendBitString(bytesFromHex("01").span(), 1U));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, encoder.appendPrintableString("not@printable"_el));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, encoder.appendIa5String("Grüezi"_el));
        const auto tagged = encoder.beginExplicit(31U);
        encoder.end(tagged);
        REQUIRE_NOTHROW(encoder.appendPrintableString("- AZaz09'()+,./:=?"_el));

        const auto malformedUtf8 = el::String{el::unittest::th::stdStringFromHex("41 C0 42")};
        REQUIRE_THROWS_AS(el::err::ParameterFailure, encoder.appendUtf8String(malformedUtf8));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, encoder.appendPrintableString(malformedUtf8));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, encoder.appendIa5String(malformedUtf8));

        auto nested = el::data::der::DerEncoder{};
        const auto outer = nested.beginSequence();
        [[maybe_unused]] const auto inner = nested.beginSequence();
        REQUIRE_THROWS_AS(el::err::LogicFailure, nested.end(outer));
    }

    void testSignedValuesAndSensitiveStorage() {
        const auto check = [&](const int64_t value, const std::string_view hex) {
            auto writer = DerEncoder{};
            writer.markAsSensitive();
            writer.appendInteger(value);
            REQUIRE(writer.encoded().isSensitive());
            REQUIRE_EQUAL(writer.encoded(), bytesFromHex(hex));
            REQUIRE(DerParser::parseOrThrow(bytesFromHex(hex)).toInteger() == value);
        };
        check(INT64_MIN, "02088000000000000000");
        check(-129, "0202ff7f");
        check(-128, "020180");
        check(-1, "0201ff");
        check(0, "020100");
        check(127, "02017f");
        check(128, "02020080");
        check(INT64_MAX, "02087fffffffffffffff");
        auto writer = DerEncoder{};
        writer.appendEnumerated(128);
        REQUIRE_EQUAL(writer.encoded(), bytesFromHex("0a020080"));
        REQUIRE(DerParser::parseOrThrow(writer.encoded()).toEnumerated() == 128);
        const auto big = DerParser::parseOrThrow(bytesFromHex("020900ffffffffffffffff"));
        REQUIRE_FALSE(big.toInteger().has_value());
        REQUIRE(big.toUnsignedInteger() == UINT64_MAX);
    }

    void testSetAndSetOfOrdering() {
        const auto byTag = bytesFromHex("3104a0008100");
        const auto byEncoding = bytesFromHex("31048100a000");
        REQUIRE(DerParser::isSetOrdered(DerParser::parseOrThrow(byTag)));
        REQUIRE_FALSE(DerParser::isSetOfOrdered(DerParser::parseOrThrow(byTag)));
        REQUIRE(DerParser::isSetOfOrdered(DerParser::parseOrThrow(byEncoding)));
        REQUIRE_FALSE(DerParser::isSetOrdered(DerParser::parseOrThrow(byEncoding)));
        auto writer = DerEncoder{};
        const auto set = writer.beginSet();
        const auto first = writer.beginExplicit(0U);
        writer.end(first);
        writer.appendImplicitPrimitive(1U, {});
        REQUIRE_THROWS_AS(el::err::ParameterFailure, writer.appendImplicitPrimitive(1U, {}));
        writer.end(set);
        REQUIRE_EQUAL(writer.encoded(), byTag);
        REQUIRE_THROWS_AS(el::err::LogicFailure, writer.end(set));
        REQUIRE_THROWS_AS(el::err::ParseError, writer.appendEncoded(bytesFromHex("30800000")));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            writer.appendEncoded(el::data::ber::BerParser::parseOrThrow(bytesFromHex("048100"))));
        REQUIRE_EQUAL(writer.encoded(), byTag);
    }
};
