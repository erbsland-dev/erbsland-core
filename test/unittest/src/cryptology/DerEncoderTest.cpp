// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/asn1/Asn1ObjectIdentifier.hpp>
#include <erbsland/cryptology/impl/DerEncoder.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/time/Date.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/time/Time.hpp>
#include <erbsland/time/TimeAmounts.hpp>
#include <erbsland/util/List.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(DerEncoder)
class DerEncoderTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
    using DerEncoder = el::cryptology::impl::DerEncoder;

public:
    void testPrimitiveValues() {
        auto encoder = DerEncoder{};
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
        const auto oid = Asn1ObjectIdentifier::fromStringOrThrow("1.2.840.113549.1.1.11"_el);
        auto first = DerEncoder{};
        first.appendObjectIdentifier(oid);
        REQUIRE_EQUAL(first.encoded(), bytesFromHex("06092a864886f70d01010b"));
        auto one = DerEncoder{};
        one.appendPositiveInteger(1U);
        auto two = DerEncoder{};
        two.appendPositiveInteger(2U);
        const auto values = el::util::List<el::ByteBlock>{
            two.encoded(),
            one.encoded(),
        };
        auto set = DerEncoder{};
        set.appendSet(values);
        REQUIRE_EQUAL(set.encoded(), bytesFromHex("3106020101020102"));
    }

    void testTimeSelection() {
        using namespace el::time;

        const auto utc = DateTime{Date::fromYearMonthDay(2049, 12, 31), Time{Hour{23}, Minute{59}, Second{58}}};
        const auto generalized = DateTime{Date::fromYearMonthDay(2050, 1, 1), Time{Hour{0}, Minute{0}, Second{1}}};
        const auto generalizedWithOffset =
            DateTime{Date::fromYearMonthDay(2050, 1, 1), Time{Hour{2}, Minute{0}, Second{1}}, Duration{Hours{2}}};
        auto encoder = DerEncoder{};
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
        auto encoder = DerEncoder{};
        const auto sequence = encoder.beginSequence();
        encoder.appendOctetString(el::ByteBlock{el::ByteLength{128U}}.span());
        encoder.end(sequence);
        const auto expectedPrefix = bytesFromHex("308183048180");
        REQUIRE_EQUAL(encoder.encoded().length(), el::ByteLength{134U});
        REQUIRE_EQUAL(encoder.encoded().slice(el::ByteIndex::zero(), expectedPrefix.length()), expectedPrefix);
    }

    void testRejectedNonCanonicalValues() {
        auto encoder = DerEncoder{};
        REQUIRE_THROWS_AS(el::err::ParameterError, encoder.appendBitString(bytesFromHex("01").span(), 1U));
        REQUIRE_THROWS_AS(el::err::ParameterError, encoder.appendPrintableString("not@printable"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, encoder.appendIa5String("Grüezi"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, encoder.beginExplicit(31U));
        REQUIRE_NOTHROW(encoder.appendPrintableString("- AZaz09'()+,./:=?"_el));

        const auto malformedUtf8 = el::String{el::unittest::th::stdStringFromHex("41 C0 42")};
        REQUIRE_THROWS_AS(el::err::ParameterError, encoder.appendUtf8String(malformedUtf8));
        REQUIRE_THROWS_AS(el::err::ParameterError, encoder.appendPrintableString(malformedUtf8));
        REQUIRE_THROWS_AS(el::err::ParameterError, encoder.appendIa5String(malformedUtf8));

        auto nested = DerEncoder{};
        const auto outer = nested.beginSequence();
        [[maybe_unused]] const auto inner = nested.beginSequence();
        REQUIRE_THROWS_AS(el::err::LogicError, nested.end(outer));
    }
};
