// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "X509TestData.hpp"

#include <erbsland/cryptology/asn1/Asn1Node.hpp>
#include <erbsland/cryptology/asn1/Asn1ObjectIdentifier.hpp>
#include <erbsland/cryptology/impl/DerParser.hpp>
#include <erbsland/cryptology/x509/X509Certificate.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <vector>

using namespace el::cryptology;
using namespace el::text::literals;
using erbsland::test::x509::certificatePem;

TESTED_TARGETS(Asn1Node Asn1ObjectIdentifier Asn1TagClass Asn1UniversalType)
class Asn1NodeTest final : public el::UnitTest {
public:
    void testCertificateTree() {
        const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
        const auto root = certificate.asn1();
        REQUIRE_FALSE(root.isEmpty());
        REQUIRE(root.tagClass() == Asn1TagClass::Universal);
        REQUIRE(root.universalType() == Asn1UniversalType::Sequence);
        REQUIRE(root.isConstructed());
        REQUIRE_EQUAL(root.childCount(), el::unit::ItemCount{3U});
        REQUIRE_EQUAL(root.encodedData(), certificate.toDer());
        REQUIRE(root.child(el::unit::ItemIndex{99U}).isEmpty());
        REQUIRE(root.child(el::unit::ItemIndex{1U}).child(el::unit::ItemIndex{0U}).toObjectIdentifier().has_value());
    }

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
            const auto certificate = X509Certificate::fromPemOrThrow(certificatePem());
            retainedNode = certificate.asn1().child(el::unit::ItemIndex{1U});
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
        REQUIRE_FALSE(Asn1ObjectIdentifier::fromString("1.02.3"_el).has_value());
        REQUIRE_FALSE(Asn1ObjectIdentifier::fromString("3.1"_el).has_value());
        REQUIRE_FALSE(Asn1ObjectIdentifier::fromString("1"_el).has_value());
    }

private:
    [[nodiscard]] static auto byteBlock(const std::vector<uint8_t> &bytes) -> el::mem::ByteBlock {
        return el::mem::ByteBlock::fromVector(bytes);
    }
    [[nodiscard]] static auto parseDer(const std::vector<uint8_t> &bytes) -> Asn1Node {
        return el::cryptology::impl::DerParser{byteBlock(bytes)}.parseDocument();
    }
};
