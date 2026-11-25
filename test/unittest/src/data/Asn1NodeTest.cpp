// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/asn1/Asn1Node.hpp>
#include <erbsland/asn1/Asn1ObjectIdentifier.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/der/DerParser.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <vector>

using namespace el::data::asn1;
using namespace el::text::literals;

TESTED_TARGETS(Asn1Node Asn1ObjectIdentifier Asn1TagClass Asn1UniversalType)
class Asn1NodeTest final : public el::UnitTest {
public:
    void testPrimitiveAndConstructedForms() {
        const auto primitive = parseDer({0x04U, 0x03U, 0xAAU, 0xBBU, 0xCCU});
        REQUIRE_FALSE(primitive.isEmpty());
        REQUIRE_FALSE(primitive.isConstructed());
        REQUIRE(primitive.universalType() == Asn1UniversalType::OctetString);
        REQUIRE_EQUAL(primitive.encodedData(), byteBlock({0x04U, 0x03U, 0xAAU, 0xBBU, 0xCCU}));
        REQUIRE_EQUAL(primitive.contentData(), byteBlock({0xAAU, 0xBBU, 0xCCU}));
        REQUIRE_EQUAL(primitive.childCount(), el::unit::ItemCount{});

        const auto constructed = parseDer({0x30U, 0x00U});
        REQUIRE(constructed.isConstructed());
        REQUIRE(constructed.universalType() == Asn1UniversalType::Sequence);
        REQUIRE(constructed.contentData().isEmpty());
        REQUIRE_EQUAL(constructed.childCount(), el::unit::ItemCount{});
    }

    void testNestedChildrenAndCow() {
        const auto root = parseDer({0x30U, 0x05U, 0x01U, 0x01U, 0xFFU, 0x05U, 0x00U});
        REQUIRE_EQUAL(root.childCount(), el::unit::ItemCount{2U});
        const auto boolean = root.child(el::unit::ItemIndex{0U}).toBoolean();
        REQUIRE(boolean.has_value());
        REQUIRE(*boolean);
        REQUIRE(root.child(el::unit::ItemIndex{1U}).universalType() == Asn1UniversalType::Null);

        auto children = root.children();
        REQUIRE_EQUAL(children.count(), el::unit::ItemCount{2U});
        children.clear();
        REQUIRE_EQUAL(children.count(), el::unit::ItemCount{});
        REQUIRE_EQUAL(root.childCount(), el::unit::ItemCount{2U});
    }

    void testHighTagNumber() {
        const auto node = parseDer({0xBFU, 0x1FU, 0x00U});
        REQUIRE(node.tagClass() == Asn1TagClass::Context);
        REQUIRE_EQUAL(node.tagNumber(), 31U);
        REQUIRE(node.isConstructed());
        REQUIRE_EQUAL(node.childCount(), el::unit::ItemCount{});
    }

    void testNodeLifetime() {
        auto retainedNode = Asn1Node{};
        {
            const auto root = parseDer({0x30, 0x05, 0x30, 0x03, 0x06, 0x01, 0x2a});
            retainedNode = root.child(el::unit::ItemIndex::zero());
        }
        REQUIRE_FALSE(retainedNode.isEmpty());
        REQUIRE(retainedNode.universalType() == Asn1UniversalType::Sequence);
        REQUIRE(retainedNode.child(el::unit::ItemIndex{0U}).toObjectIdentifier().has_value());
    }

    void testObjectIdentifierDer() {
        const auto node = parseDer({0x06U, 0x09U, 0x2AU, 0x86U, 0x48U, 0x86U, 0xF7U, 0x0DU, 0x01U, 0x01U, 0x0BU});
        const auto first = node.toObjectIdentifier();
        const auto second = node.toObjectIdentifier();
        REQUIRE(first.has_value());
        REQUIRE(second.has_value());
        REQUIRE_EQUAL(first->toString(), "1.2.840.113549.1.1.11"_el);
        REQUIRE_EQUAL(second->toString(), first->toString());
        REQUIRE_THROWS_AS(el::err::ParseError, parseDer({0x06U, 0x00U}));
        REQUIRE_THROWS_AS(el::err::ParseError, parseDer({0x06U, 0x02U, 0x80U, 0x00U}));
        REQUIRE_THROWS_AS(el::err::ParseError, parseDer({0x06U, 0x01U, 0x81U}));
    }

    void testCanonicalSetOrder() {
        REQUIRE_NOTHROW(parseDer({0x31U, 0x06U, 0x02U, 0x01U, 0x01U, 0x02U, 0x01U, 0x02U}));
        REQUIRE_THROWS_AS(el::err::ParseError, parseDer({0x31U, 0x06U, 0x02U, 0x01U, 0x02U, 0x02U, 0x01U, 0x01U}));
    }

    void testObjectIdentifierText() {
        const auto oid = Asn1ObjectIdentifier::fromStringOrThrow("1.2.840.113549.1.1.11"_el);
        REQUIRE_EQUAL(oid.toString(), "1.2.840.113549.1.1.11"_el);
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("1.02.3"_el).has_value());
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("3.1"_el).has_value());
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("1"_el).has_value());
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("1,2"_el).has_value());
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("1 2"_el).has_value());
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("1/2"_el).has_value());
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("1..2"_el).has_value());
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("1.2."_el).has_value());
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("1.18446744073709551616"_el).has_value());
        REQUIRE(el::data::asn1::Asn1ObjectIdentifier::fromString("2.18446744073709551535"_el).has_value());
        REQUIRE_FALSE(el::data::asn1::Asn1ObjectIdentifier::fromString("2.18446744073709551536"_el).has_value());
    }

    void testSharedStorageAndNestedCoordinates() {
        const auto storage = byteBlock({0xaa, 0xbb, 0x30, 0x06, 0x04, 0x04, 0x30, 0x02, 0x05, 0x00, 0xcc});
        const auto document = storage.slice(el::ByteIndex{2}, el::ByteCount{8});
        const auto outer = el::data::der::DerParser::parseOrThrow(document);
        const auto payload = outer.child(el::ItemIndex::zero());
        REQUIRE_EQUAL(payload.encodedByteIndex(), el::ByteIndex{2});
        REQUIRE_EQUAL(payload.contentByteIndex(), el::ByteIndex{4});
        REQUIRE(payload.contentData().span().data() == document.span().data() + 4);
        const auto inner = el::data::der::DerParser::parseContentOrThrow(payload);
        REQUIRE_EQUAL(inner.encodedByteIndex(), el::ByteIndex{4});
        REQUIRE_EQUAL(inner.child(el::ItemIndex::zero()).contentByteIndex(), el::ByteIndex{8});
        const auto malformed = parseDer({0x30, 0x06, 0x04, 0x04, 0x30, 0x02, 0x04, 0x02});
        try {
            static_cast<void>(el::data::der::DerParser::parseContentOrThrow(malformed.child(el::ItemIndex::zero())));
            REQUIRE(false);
        } catch (const el::err::ParseError &error) {
            REQUIRE_EQUAL(error.byteIndex(), el::ByteIndex{8});
        }
        const auto context = Asn1Tag{Asn1TagClass::Context, 0U};
        const auto bits = parseDer({0x80, 0x02, 0x03, 0xa0}).toBitString(context);
        REQUIRE(bits.has_value());
        REQUIRE_EQUAL(bits->unusedBits, 3U);
        REQUIRE_EQUAL(bits->bytes, byteBlock({0xa0}));
        REQUIRE_FALSE(parseDer({0x80, 0x02, 0x03, 0xa1}).toBitString(context));
        REQUIRE(parseDer({0x80, 0x01, 0x2a}).toObjectIdentifier(context)->toString() == "1.2"_el);
    }

private:
    [[nodiscard]] static auto byteBlock(const std::vector<uint8_t> &bytes) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromVector(bytes);
    }
    [[nodiscard]] static auto parseDer(const std::vector<uint8_t> &bytes) -> Asn1Node {
        return el::data::der::DerParser::parseOrThrow(byteBlock(bytes));
    }
};
