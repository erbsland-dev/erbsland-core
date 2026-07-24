// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../ConfTestHelper.hpp"

#include <erbsland/conf/impl/constants/Limits.hpp>
#include <erbsland/conf/Name.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <unordered_set>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(Name)
class NameTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    Name name;

    void testEmpty() {
        name = {};
        REQUIRE(name.empty());
        REQUIRE(name.isRegular());
        REQUIRE(name.asText().isEmpty());
        REQUIRE(name.asIndex() == 0);
        REQUIRE(name.pathTextSize() == 0);
        REQUIRE(name.toPathText().isEmpty());
        REQUIRE_EQUAL(name.type(), NameType::Regular);
    }

    void testCreateRegular() {
        name = Name::createRegular("server"_el);
        REQUIRE(name.isRegular());
        REQUIRE(name.asText() == el::text::String{"server"_el});
        REQUIRE(name.asIndex() == 0);
        REQUIRE(name.pathTextSize() == 6);
        REQUIRE(name.toPathText() == el::text::String{"server"_el});
        REQUIRE_EQUAL(name.type(), NameType::Regular);
    }

    void testCreateRegularMeta() {
        name = Name::createRegular("@version"_el);
        REQUIRE(name.isRegular());
        REQUIRE(name.asText() == el::text::String{"@version"_el});
        REQUIRE(name.asIndex() == 0);
        REQUIRE(name.pathTextSize() == 8);
        REQUIRE(name.toPathText() == el::text::String{"@version"_el});
        REQUIRE_EQUAL(name.type(), NameType::Regular);
    }

    void testCreateText() {
        // move
        name = Name::createText("text"_el);
        REQUIRE(name.isText());
        REQUIRE(name.asText() == el::text::String{"text"_el});
        REQUIRE(name.asIndex() == 0);
        REQUIRE(name.pathTextSize() == 6); // "text"
        REQUIRE(name.toPathText() == el::text::String{"\"text\""_el});
        REQUIRE_EQUAL(name.type(), NameType::Text);

        // copy
        const auto text = el::text::String{"text"_el};
        name = Name::createText(text);
        REQUIRE(name.isText());
        REQUIRE(name.asText() == el::text::String{"text"_el});
        REQUIRE(name.asIndex() == 0);
        REQUIRE(name.pathTextSize() == 6); // "text"
        REQUIRE(name.toPathText() == el::text::String{"\"text\""_el});
        REQUIRE_EQUAL(name.type(), NameType::Text);
    }

    void testCreateIndex() {
        name = Name::createIndex(42);
        REQUIRE(name.isIndex());
        REQUIRE_EQUAL(name.asText(), el::text::String{"42"_el});
        REQUIRE_EQUAL(name.asIndex(), 42);
        REQUIRE_EQUAL(name.pathTextSize(), 4);
        REQUIRE_EQUAL(name.toPathText(), el::text::String{"[42]"_el});
        REQUIRE_EQUAL(name.type(), NameType::Index);
    }

    void testCreateTextIndex() {
        name = Name::createTextIndex(3);
        REQUIRE(name.isTextIndex());
        REQUIRE_EQUAL(name.asText(), el::text::String{"3"_el});
        REQUIRE_EQUAL(name.asIndex(), 3);
        REQUIRE_EQUAL(name.pathTextSize(), 5);
        REQUIRE_EQUAL(name.toPathText(), el::text::String{"\"\"[3]"_el});
        REQUIRE_EQUAL(name.type(), NameType::TextIndex);
    }

    void testComparison() {
        name = Name::createRegular("server"_el);
        REQUIRE(name == Name::createRegular("server"_el));
        REQUIRE(name != Name::createRegular("server1"_el));
        REQUIRE(name != Name::createRegular("server2"_el));
        REQUIRE(name != Name::createIndex(42));
        REQUIRE(name != Name::createText("server"_el));
        REQUIRE(name != Name::createTextIndex(3));
    }

    void testHash() {
        name = Name::createRegular("server"_el);
        REQUIRE_EQUAL(name.hash(), Name::createRegular("server"_el).hash());
        REQUIRE_NOT_EQUAL(name.hash(), Name::createRegular("server1"_el).hash());

        REQUIRE_EQUAL(std::hash<Name>{}(name), std::hash<Name>{}(Name::createRegular("server"_el)));

        std::unordered_set<Name> names;
        names.insert(Name::createRegular("server"_el));
        names.insert(Name::createRegular("server2"_el));
        names.insert(Name::createText("server"_el));
        names.insert(Name::createIndex(32));
        REQUIRE(names.contains(Name::createRegular("server"_el)));
        REQUIRE(names.contains(Name::createRegular("server2"_el)));
        REQUIRE(names.contains(Name::createText("server"_el)));
        REQUIRE(names.contains(Name::createIndex(32)));
    }

    void testFormat() {
        name = Name::createRegular("server"_el);
        auto text = std::format("*{}*", name);
        REQUIRE_EQUAL(text, std::string("*server*"));
    }

    void testNormalize() {
        auto text = Name::normalize("A Valid Name 99 12"_el);
        REQUIRE_EQUAL(text, el::text::String{"a_valid_name_99_12"_el});
        name = Name::createRegular("Example Name123"_el);
        REQUIRE_EQUAL(name.asText(), el::text::String{"example_name123"_el});
        name = Name::createText("Example Name123"_el);
        REQUIRE_EQUAL(name.asText(), el::text::String{"Example Name123"_el});
    }

    void testMalformedRegularNames() {
        // empty names aren't allowed.
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular(""_el));
        // names must not exceed maximum length.
        const auto longName =
            el::text::StringEditor::fromCharacter(el::text::Char{U'a'}, el::unit::CpLength{limits::maxNameLength + 1});
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular(longName));
        // names must not start with space or underscore.
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("_name"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular(" name"_el));
        // names must not end with space or underscore.
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("name_"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("name "_el));
        // name must not contain more than one word separator.
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("one__two"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("one  two"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("one _two"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("one_ two"_el));
        // name must not start with a decimal digit.
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("0name"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("9name"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("@0name"_el));
        // name must be limited to a-z, 0-9, _ and space.
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("name[]name"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("[name]"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("name.name"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular(".name"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("name."_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("näme"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("äbc"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("name→name"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("→name"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("name→"_el));
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("nａme"_el)); // full-width 'a'
        // encoding errors.
        REQUIRE_THROWS_AS(ConfError, name = Name::createRegular("name\xffname"_el));
    }

    void testMalformedTextNames() {
        // empty text-names aren't allowed.
        REQUIRE_THROWS_AS(ConfError, name = Name::createText(""_el));
        // names must not exceed the maximum length.
        const auto longName = el::text::StringEditor::fromCharacter(
            el::text::Char{U'a'},
            el::unit::CpLength{limits::maxLineLength + 20}); // +20 because of detection tolerance.
        REQUIRE_THROWS_AS(ConfError, name = Name::createText(longName));
        // Illegal code-points: zero is not allowed
        REQUIRE_THROWS_AS(
            ConfError, name = Name::createText(el::text::String{std::u8string_view{u8"Name \x00 Name", 11}}));
    }

    void testInternalView() {
        // testing the internal view, ensure debugging tools work flawlessly.
        name = Name::createRegular("server"_el);
        auto text = internalView(name)->toString();
        REQUIRE(text.contains("Regular"_el));
        REQUIRE(text.contains("server"_el));
        name = Name::createText("server"_el);
        text = internalView(name)->toString();
        REQUIRE(text.contains("Text"_el));
        REQUIRE(text.contains("server"_el));
        name = Name::createIndex(1234);
        text = internalView(name)->toString();
        REQUIRE(text.contains("Index"_el));
        REQUIRE(text.contains("1234"_el));
    }

    void testNameOrder() {
        WITH_CONTEXT(requireAllOperators(
            Name::createRegular("anna"_el),
            Name::createRegular("bert"_el),
            Name::createRegular("zoe"_el),
            Name::createRegular("anna"_el),
            Name::createRegular("bert"_el),
            Name::createRegular("zoe"_el)));
    }
};
