// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Asn1TestHelper.hpp"

#include <erbsland/ber/BerParser.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/der/DerParser.hpp>
#include <erbsland/OutOfRangeFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/ParseError.hpp>

using namespace el::data::asn1;
using namespace el::data::ber;
using el::data::der::DerParser;
#include <erbsland/ber/BerEncoder.hpp>
TESTED_TARGETS(BerParser BerEncoder)
class BerCodecTest final : public UNITTEST_SUBCLASS(Asn1TestHelper) {
public:
    void testUnbind() {
        using namespace el::data;
        const auto fixture = bytesFromHex("30050201014200");
        auto writer = ber::BerEncoder{};
        const auto root = writer.beginSequence();
        writer.appendInteger(1);
        writer.appendNull({asn1::Asn1TagClass::Application, 2});
        writer.end(root);
        REQUIRE_EQUAL(writer.encoded(), fixture);
        const auto parsed = BerParser::parseOrThrow(fixture);
        REQUIRE(parsed.child(el::ItemIndex::zero()).toInteger() == 1);
        const auto operation = parsed.child(el::ItemIndex::one());
        REQUIRE(operation.matches({Asn1TagClass::Application, 2}, false));
        REQUIRE(operation.isNull({Asn1TagClass::Application, 2}));
    }

    void testBindRequest() {
        const auto fixture = bytesFromHex("300e0201016009020103040080027077");
        const auto root = BerParser::parseOrThrow(fixture);
        REQUIRE(root.child(el::ItemIndex::zero()).toInteger() == 1);
        const auto bind = root.child(el::ItemIndex::one());
        REQUIRE(bind.matches({Asn1TagClass::Application, 0U}, true));
        REQUIRE(bind.child(el::ItemIndex::zero()).toInteger() == 3);
        REQUIRE(bind.child(el::ItemIndex::one()).toOctetString() == el::ByteBlock{});
        const auto authentication = Asn1Tag{Asn1TagClass::Context, 0U};
        REQUIRE(bind.child(el::ItemIndex{2U}).toOctetString(authentication) == bytesFromHex("7077"));

        auto writer = BerEncoder{};
        const auto message = writer.beginSequence();
        writer.appendInteger(1);
        const auto operation = writer.beginConstructed({Asn1TagClass::Application, 0U});
        writer.appendInteger(3);
        writer.appendOctetString({});
        writer.appendOctetString(bytesFromHex("7077").span(), authentication);
        writer.end(operation);
        writer.end(message);
        REQUIRE_EQUAL(writer.encoded(), fixture);
    }

    void testSearchRequest() {
        const auto fixture = bytesFromHex("3020020102631b04000a01020a01000201000201000101008702636e30040402636e");
        const auto operation = BerParser::parseOrThrow(fixture).child(el::ItemIndex::one());
        REQUIRE(operation.matches({Asn1TagClass::Application, 3U}, true));
        REQUIRE(operation.child(el::ItemIndex{1U}).toEnumerated() == 2);
        REQUIRE(operation.child(el::ItemIndex{2U}).toEnumerated() == 0);
        REQUIRE(operation.child(el::ItemIndex{5U}).toBoolean() == false);
        const auto present = Asn1Tag{Asn1TagClass::Context, 7U};
        const auto cn = bytesFromHex("636e");
        REQUIRE(operation.child(el::ItemIndex{6U}).toOctetString(present) == cn);
        REQUIRE(operation.child(el::ItemIndex{7U}).child(el::ItemIndex::zero()).toOctetString() == cn);

        auto writer = BerEncoder{};
        const auto message = writer.beginSequence();
        writer.appendInteger(2);
        const auto search = writer.beginConstructed({Asn1TagClass::Application, 3U});
        writer.appendOctetString({});
        writer.appendEnumerated(2);
        writer.appendEnumerated(0);
        writer.appendInteger(0);
        writer.appendInteger(0);
        writer.appendBoolean(false);
        writer.appendPrimitive(present, cn.span());
        const auto attributes = writer.beginSequence();
        writer.appendOctetString(cn.span());
        writer.end(attributes);
        writer.end(search);
        writer.end(message);
        REQUIRE_EQUAL(writer.encoded(), fixture);
    }

    void testSearchResultWithBinaryValues() {
        const auto fixture = bytesFromHex("3019020102641404003010300e0402636e31080402ff00040200ff");
        const auto entry = BerParser::parseOrThrow(fixture).child(el::ItemIndex::one());
        REQUIRE(entry.matches({Asn1TagClass::Application, 4U}, true));
        const auto values = entry.child(el::ItemIndex::one()).child(el::ItemIndex::zero()).child(el::ItemIndex::one());
        REQUIRE(values.child(el::ItemIndex::zero()).toOctetString() == bytesFromHex("ff00"));
        REQUIRE(values.child(el::ItemIndex::one()).toOctetString() == bytesFromHex("00ff"));
        REQUIRE_FALSE(DerParser::parse(fixture).has_value());

        auto writer = BerEncoder{};
        const auto message = writer.beginSequence();
        writer.appendInteger(2);
        const auto result = writer.beginConstructed({Asn1TagClass::Application, 4U});
        writer.appendOctetString({});
        const auto attributes = writer.beginSequence();
        const auto attribute = writer.beginSequence();
        writer.appendOctetString(bytesFromHex("636e").span());
        const auto set = writer.beginSetOf();
        writer.appendOctetString(bytesFromHex("ff00").span());
        writer.appendOctetString(bytesFromHex("00ff").span());
        writer.end(set);
        writer.end(attribute);
        writer.end(attributes);
        writer.end(result);
        writer.end(message);
        REQUIRE_EQUAL(writer.encoded(), fixture);
    }

    void testFramingAndNonminimalDefiniteLengths() {
        const auto frame = bytesFromHex("3084000000050201014200");
        for (auto size = el::ByteCount{}; size < frame.length(); ++size) {
            REQUIRE_FALSE(BerParser::frameLength(frame.slice(el::ByteIndex::zero(), size)).has_value());
        }
        REQUIRE(BerParser::frameLength(frame) == frame.length());
        REQUIRE(BerParser::parse(frame).has_value());
        REQUIRE_FALSE(DerParser::parse(frame).has_value());
        auto joined = el::ByteBlockEditor{frame};
        joined.append(frame);
        const auto limits = Asn1Limits{.maximumLength = frame.length()};
        REQUIRE(BerParser::frameLength(joined, limits) == frame.length());
        REQUIRE_FALSE(BerParser::parse(joined, limits).has_value());
        REQUIRE_FALSE(BerParser::parse(joined).has_value());
        REQUIRE(
            BerParser::frameLength(joined.slice(el::ByteIndex::end(frame.length()), frame.length())) == frame.length());
        REQUIRE_THROWS_AS(el::err::ParseError, BerParser::frameLength(bytesFromHex("04820100"), limits));
        REQUIRE_THROWS_AS(el::err::ParseError, BerParser::frameLength(bytesFromHex("04ff")));
        REQUIRE_THROWS_AS(el::err::ParseError, BerParser::frameLength(bytesFromHex("0480")));
        REQUIRE_THROWS_AS(el::err::ParseError, BerParser::frameLength(bytesFromHex("1f80")));
        REQUIRE_FALSE(BerParser::parse(bytesFromHex("2403040100")).has_value());
        REQUIRE_FALSE(BerParser::parse(bytesFromHex("010101")).has_value());
        REQUIRE_FALSE(BerParser::parse(bytesFromHex("c000")).has_value());
        REQUIRE_FALSE(BerParser::parseOrThrow(bytesFromHex("420100")).isNull({Asn1TagClass::Application, 2}));
        REQUIRE_FALSE(BerParser::parse(bytesFromHex("0a020001")).has_value());
        REQUIRE_FALSE(BerParser::parse(bytesFromHex("300204020000")).has_value());
    }

    void testBudgetsAndImplicitPrimitives() {
        const auto twoNodes = bytesFromHex("3003020100");
        REQUIRE(BerParser::parse(twoNodes, {.maximumDepth = el::ItemCount{2}, .maximumNodes = el::ItemCount{2}}));
        REQUIRE_FALSE(BerParser::parse(twoNodes, {.maximumDepth = el::ItemCount::one()}));
        REQUIRE_FALSE(BerParser::parse(twoNodes, {.maximumNodes = el::ItemCount::one()}));
        auto writer = BerEncoder{{.maximumLength = el::ByteCount{5}, .maximumNodes = el::ItemCount{2}}};
        const auto outer = writer.beginSequence();
        REQUIRE_THROWS_AS(el::err::OutOfRangeFailure, writer.appendOctetString(twoNodes.span()));
        writer.appendInteger(0);
        REQUIRE_THROWS_AS(el::err::OutOfRangeFailure, writer.appendNull());
        writer.end(outer);
        REQUIRE_EQUAL(writer.encoded(), twoNodes);

        const auto context = Asn1Tag{Asn1TagClass::Context, UINT32_MAX};
        auto implicit = BerEncoder{};
        implicit.appendBoolean(true, context);
        REQUIRE_EQUAL(implicit.encoded(), bytesFromHex("9f8fffffff7f01ff"));
        REQUIRE(BerParser::parseOrThrow(implicit.encoded()).toBoolean(context) == true);
        REQUIRE_FALSE(BerParser::parseOrThrow(bytesFromHex("800101")).toBoolean({Asn1TagClass::Context, 0}));
        REQUIRE_FALSE(BerParser::parseOrThrow(bytesFromHex("8000")).toUnsignedInteger({Asn1TagClass::Context, 0}));
        REQUIRE_FALSE(BerParser::parseOrThrow(bytesFromHex("040101")).toInteger(Asn1UniversalType::OctetString));
        REQUIRE_THROWS_AS(el::err::ParameterFailure, implicit.appendInteger(1, Asn1UniversalType::Boolean));
    }
};
