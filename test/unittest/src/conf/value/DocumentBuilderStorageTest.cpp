// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/impl/value/DocumentBuilderStorage.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::conf;
using namespace el::text::literals;
using impl::DocumentBuilderStorage;

TESTED_TARGETS(DocumentBuilderStorage DocumentBuilder)
class DocumentBuilderStorageTest final : public el::UnitTest {
public:
    // NOTES:
    // This unit test is just a supplement to the tests in the `DocumentBuilderImplTest`.
    // The unit test focuses on low-level interface tests, which aren't possible via the public interface
    // in the builder class.

    void testConstruction() {
        DocumentBuilderStorage storage;
        // manually add a section.
        auto namePath = NamePath::fromText("main"_el);
        auto location = Location{
            SourceIdentifier::createForFile("file.elcl"_el),
            el::unit::CodeLocation{el::unit::LineIndex{0U}, el::unit::ColumnIndex{0U}}};
        auto section = impl::Value::createSectionWithNames();
        section->setName(namePath.at(0));
        storage.addChildValue(nullptr, namePath, location, section);
        // verify its basic get and reset mechanism.
        auto doc1 = storage.getDocumentAndReset();
        auto doc2 = storage.getDocumentAndReset();
        REQUIRE(doc1 != nullptr);
        REQUIRE(doc2 != nullptr);
        REQUIRE(doc1 != doc2);
        REQUIRE(doc1->value(NamePath::fromText("main"_el)) != nullptr);
        REQUIRE(doc2->value(NamePath::fromText("main"_el)) == nullptr);
    }

    void testApiErrors() {
        DocumentBuilderStorage storage;
        auto location = Location{
            SourceIdentifier::createForFile("file.elcl"_el),
            el::unit::CodeLocation{el::unit::LineIndex{0U}, el::unit::ColumnIndex{0U}}};
        auto sectionNamePath = NamePath::fromText("main"_el);
        auto section = impl::Value::createSectionWithNames();
        section->setName(sectionNamePath.back());
        auto valueNamePath = NamePath::fromText("main.value_1"_el);
        auto value = impl::Value::createInteger(1);
        value->setName(valueNamePath.back());

        REQUIRE_THROWS(storage.updateLastSection(nullptr, sectionNamePath));
        REQUIRE_THROWS(storage.updateLastSection(section, NamePath{}));

        REQUIRE_THROWS(storage.addChildValue(nullptr, valueNamePath, location, value));
        REQUIRE_THROWS(storage.addChildValue(section, NamePath{}, location, value));
        REQUIRE_THROWS(storage.addChildValue(section, valueNamePath, location, nullptr));
    }

    void testAddingValueToSectionList() {
        DocumentBuilderStorage storage;
        auto location = Location{
            SourceIdentifier::createForFile("file.elcl"_el),
            el::unit::CodeLocation{el::unit::LineIndex{0U}, el::unit::ColumnIndex{0U}}};
        auto sectionNamePath = NamePath::fromText("server"_el);
        auto sectionList = impl::Value::createSectionList();
        sectionList->setName(sectionNamePath.back());
        auto section = impl::Value::createSectionWithNames();
        auto valueNamePath = NamePath::fromText("server.value_1"_el);
        auto value = impl::Value::createInteger(1);
        value->setName(valueNamePath.back());

        REQUIRE_NOTHROW(storage.addChildValue(nullptr, sectionNamePath, location, sectionList));
        REQUIRE_NOTHROW(storage.addChildValue(sectionList, sectionNamePath, location, section));
        REQUIRE_THROWS(storage.addChildValue(sectionList, valueNamePath, location, value));
    }

    void testAddingValueToLeaf() {
        DocumentBuilderStorage storage;
        auto location = Location{
            SourceIdentifier::createForFile("file.elcl"_el),
            el::unit::CodeLocation{el::unit::LineIndex{0U}, el::unit::ColumnIndex{0U}}};
        auto sectionNamePath = NamePath::fromText("main"_el);
        auto section = impl::Value::createSectionWithNames();
        section->setName(sectionNamePath.back());
        section->setLocation(location);
        auto value1NamePath = NamePath::fromText("main.value_1"_el);
        auto value1 = impl::Value::createInteger(1);
        value1->setName(value1NamePath.back());
        auto value2NamePath = NamePath::fromText("main.value_1.value_2"_el);
        auto value2 = impl::Value::createInteger(1);
        value2->setName(value1NamePath.back());

        REQUIRE_NOTHROW(storage.addChildValue(nullptr, sectionNamePath, location, section));
        REQUIRE_NOTHROW(storage.addChildValue(section, value1NamePath, location, value1));
        REQUIRE_THROWS(storage.addChildValue(value1, value2NamePath, location, value2));
    }

    void testInvalidEmptySectionList() {
        DocumentBuilderStorage storage;
        auto location = Location{
            SourceIdentifier::createForFile("file.elcl"_el),
            el::unit::CodeLocation{el::unit::LineIndex{0U}, el::unit::ColumnIndex{0U}}};
        auto sectionNamePath = NamePath::fromText("server"_el);
        auto sectionList = impl::Value::createSectionList();
        sectionList->setName(sectionNamePath.back());
        auto valueNamePath = NamePath::fromText("server.value_1"_el);

        REQUIRE_NOTHROW(storage.addChildValue(nullptr, sectionNamePath, location, sectionList));
        // the list is there, but there is no section in the list.
        impl::ValuePtr result1;
        impl::ValuePtr result2;
        REQUIRE_THROWS(result1 = storage.resolveForValue(valueNamePath, location));
        REQUIRE_THROWS(std::tie(result1, result2) = storage.resolveForSection(valueNamePath, location));
    }

    void testLocationIsSetInValue() {
        DocumentBuilderStorage storage;
        auto location = Location{
            SourceIdentifier::createForFile("file.elcl"_el),
            el::unit::CodeLocation{el::unit::LineIndex{0U}, el::unit::ColumnIndex{0U}}};
        auto sectionNamePath = NamePath::fromText("main"_el);
        auto section = impl::Value::createSectionWithNames();
        section->setName(sectionNamePath.back());
        auto valueNamePath = NamePath::fromText("main.value_1"_el);
        auto value = impl::Value::createInteger(1);
        value->setName(valueNamePath.back());

        REQUIRE(section->location().isUndefined());
        REQUIRE_NOTHROW(storage.addChildValue(nullptr, sectionNamePath, location, section));
        REQUIRE(section->location() == location);
        REQUIRE(value->location().isUndefined());
        REQUIRE_NOTHROW(storage.addChildValue(section, valueNamePath, location, value));
        REQUIRE(value->location() == location);
    }

    void testParentIsSetInValue() {
        DocumentBuilderStorage storage;
        auto location = Location{
            SourceIdentifier::createForFile("file.elcl"_el),
            el::unit::CodeLocation{el::unit::LineIndex{0U}, el::unit::ColumnIndex{0U}}};
        auto sectionNamePath = NamePath::fromText("main"_el);
        auto section = impl::Value::createSectionWithNames();
        section->setName(sectionNamePath.back());
        auto valueNamePath = NamePath::fromText("main.value_1"_el);
        auto value = impl::Value::createInteger(1);
        value->setName(valueNamePath.back());

        REQUIRE(section->parent() == nullptr);
        REQUIRE_NOTHROW(storage.addChildValue(nullptr, sectionNamePath, location, section));
        REQUIRE(section->parent() != nullptr);
        REQUIRE(section->parent()->isDocument());
        REQUIRE(value->parent() == nullptr);
        REQUIRE_NOTHROW(storage.addChildValue(section, valueNamePath, location, value));
        REQUIRE(value->parent() == section);
    }
};
