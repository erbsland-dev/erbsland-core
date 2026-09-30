// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/data/xml/XmlDocument.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
using namespace el::data::xml;
TESTED_TARGETS(XmlDocument XmlNode XmlAttribute XmlParseOptions XmlFormatOptions)

class XmlDocumentTest final : public el::UnitTest {
public:
    void testElementConvenienceAndRoundTrip() {
        auto document = XmlDocument{};
        const auto root = document.addRoot("score"_el);
        root->setAttribute("lang"_el, "tr"_el);
        root->addElement("tempo"_el, "Hızlı & canlı"_el);
        const auto encoded = document.toString();
        REQUIRE_EQUAL(encoded, "<score lang=\"tr\"><tempo>Hızlı &amp; canlı</tempo></score>"_el);
        const auto parsed = XmlDocument::fromStringOrThrow(encoded);
        REQUIRE_EQUAL(parsed.root()->children().count(), el::unit::ItemCount{1U});
        REQUIRE_EQUAL(
            parsed.root()->children().getRefOrThrow(el::unit::ItemIndex{})->textContentOrThrow(), "Hızlı & canlı"_el);
    }

    void testMixedContentAndUnresolvedEntity() {
        const auto source =
            "<?xml version=\"1.0\"?><!DOCTYPE score [<!ENTITY author \"Ada\">]><score a=\"x&amp;y\">A<![CDATA[B]]>&author;<!-- note --><?play slow?></score>"_el;
        const auto document = XmlDocument::fromStringOrThrow(source);
        REQUIRE(document.root() != nullptr);
        REQUIRE_EQUAL(document.root()->getAttribute("a"_el).value(), "x&y"_el);
        REQUIRE_FALSE(document.root()->textContent().has_value());
        REQUIRE_THROWS_AS(el::err::LogicError, document.root()->textContentOrThrow());
        REQUIRE_EQUAL(
            XmlDocument::fromStringOrThrow(document.toString()).root()->children().count(), el::unit::ItemCount{5U});
    }

    void testMalformedAndLimits() {
        REQUIRE_FALSE(XmlDocument::fromString("<a><b></a>"_el).has_value());
        REQUIRE_FALSE(XmlDocument::fromString("<a x=\"1\" x=\"2\"/>"_el).has_value());
        REQUIRE_FALSE(XmlDocument::fromString("<a><!-- bad--inside --></a>"_el).has_value());
        REQUIRE_FALSE(XmlDocument::fromString("<a>"_el).has_value());
        REQUIRE_FALSE(
            XmlDocument::fromString("<a/>"_el, XmlParseOptions{}.setMaximumInputLength(el::unit::ByteLength{3U}))
                .has_value());
        REQUIRE_FALSE(XmlDocument::fromString("&broken;<a/>"_el).has_value());
        REQUIRE_FALSE(XmlDocument::fromString("<a>&#0;</a>"_el).has_value());
        REQUIRE_FALSE(XmlDocument::fromString("<a x=\"&;\"/>"_el).has_value());
        REQUIRE_FALSE(XmlDocument::fromString("<a x=\"&#0;\"/>"_el).has_value());
        REQUIRE_FALSE(XmlDocument::fromString("<?Xml?><a/>"_el).has_value());
        REQUIRE_FALSE(
            XmlDocument::fromString(
                "<a>abcd</a>"_el, XmlParseOptions{}.setMaximumStringLength(el::unit::ByteLength{3U}))
                .has_value());
        REQUIRE_FALSE(
            XmlDocument::fromString("<long/>"_el, XmlParseOptions{}.setMaximumStringLength(el::unit::ByteLength{3U}))
                .has_value());
    }

    void testDeclarationsAndNamespaces() {
        const auto source = "<?xml-stylesheet href=\"a.css\"?><!DOCTYPE r [<!-- ]> -->"
                            "<!ENTITY e \"x\">]><ns:r xmlns:ns=\"urn:r\">&e;</ns:r>"_el;
        const auto document = XmlDocument::fromStringOrThrow(source);
        REQUIRE_EQUAL(document.root()->name(), "ns:r"_el);
        REQUIRE_EQUAL(document.root()->getAttribute("xmlns:ns"_el).value(), "urn:r"_el);
        REQUIRE_FALSE(document.root()->textContent().has_value());
        REQUIRE_EQUAL(XmlDocument::fromStringOrThrow(document.toString()).root()->name(), "ns:r"_el);
    }

    void testSerializerRejectsInvalidStructure() {
        auto document = XmlDocument{};
        auto root = document.addRoot("ritim"_el);
        root->add(XmlNode::createDocType("<!DOCTYPE ritim>"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, document.toString());
        auto second = XmlDocument{};
        second.addRoot("a"_el);
        second.add(XmlNode::createElement("b"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, second.toString());
    }
};
