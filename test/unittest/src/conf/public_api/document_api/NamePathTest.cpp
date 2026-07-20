// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/NamePath.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <type_traits>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(NamePath)
class NamePathTest final : public el::UnitTest {
public:
    NamePath namePath;

    auto additionalErrorMessages() -> std::string override {
        try {
            return std::format(
                "namePath:\n{}", el::text::StringConverter{internalView(namePath)->toString(2)}.toStdString());
        } catch (...) {
            return "Unexpected exception thrown";
        }
    }

    void testEmpty() {
        namePath = {};
        REQUIRE(namePath.empty());
        REQUIRE_EQUAL(namePath.size(), 0);
    }

    void verifyConstruction(const el::text::String &name) {
        REQUIRE_FALSE(namePath.empty());
        REQUIRE_EQUAL(namePath.size(), 1);
        REQUIRE_EQUAL(namePath.at(0), Name::createRegular(name));
        REQUIRE_EQUAL(namePath.toText(), name);
    }

    void testConstruction() {
        const auto name = Name::createRegular("server"_el);
        namePath = name; // implicit, copy
        WITH_CONTEXT(verifyConstruction("server"_el));

        namePath = Name::createRegular("value"_el); // implicit, move
        WITH_CONTEXT(verifyConstruction("value"_el));

        namePath = NamePath{Name::createRegular("tree"_el)}; // explicit, move
        WITH_CONTEXT(verifyConstruction("tree"_el));

        const auto nameList = NameList{Name::createRegular("worker"_el)};
        namePath = NamePath{nameList}; // explicit, list
        WITH_CONTEXT(verifyConstruction("worker"_el));

        namePath = NamePath{std::span{nameList}}; // explicit, span
        WITH_CONTEXT(verifyConstruction("worker"_el));

        namePath = NamePath{nameList.begin(), nameList.end()}; // explicit, iterator
        WITH_CONTEXT(verifyConstruction("worker"_el));
    }

    void testNamesAccess() {
        namePath = NamePath{{Name::createRegular("server"_el), Name::createRegular("worker"_el)}};
        const auto expectedNameList = NameList{{Name::createRegular("server"_el), Name::createRegular("worker"_el)}};
        REQUIRE_EQUAL(namePath.size(), 2);
        REQUIRE_EQUAL(namePath.at(0), Name::createRegular("server"_el));
        REQUIRE_EQUAL(namePath.at(1), Name::createRegular("worker"_el));
        REQUIRE_EQUAL(namePath.front(), Name::createRegular("server"_el));
        REQUIRE_EQUAL(namePath.back(), Name::createRegular("worker"_el));
        REQUIRE_EQUAL(namePath.view()[0], Name::createRegular("server"_el));
        REQUIRE_EQUAL(namePath.view()[1], Name::createRegular("worker"_el));
        REQUIRE_EQUAL(namePath.size(), expectedNameList.size());
        std::size_t index = 0;
        for (const auto &name : namePath) {
            REQUIRE(index < expectedNameList.size())
            REQUIRE_EQUAL(name, expectedNameList.at(index));
            ++index;
        }
    }

    void testParent() {
        namePath = NamePath{{Name::createRegular("server"_el), Name::createRegular("worker"_el)}};
        namePath = namePath.parent();
        REQUIRE_EQUAL(namePath.size(), 1);
        REQUIRE_EQUAL(namePath.at(0), Name::createRegular("server"_el));
        namePath = namePath.parent();
        REQUIRE_EQUAL(namePath.size(), 0);
        namePath = namePath.parent(); // calling on an empty path shouldn't be a problem.
        REQUIRE_EQUAL(namePath.size(), 0);
    }

    void testFind() {
        static_assert(std::is_same_v<NamePath::Index, std::size_t>);
        static_assert(std::is_same_v<NamePath::Count, std::size_t>);

        namePath = NamePath{
            {Name::createRegular("server"_el),
                Name::createRegular("worker"_el),
                Name::createIndex(12),
                Name::createRegular("worker"_el)}};

        REQUIRE_EQUAL(namePath.find(Name::createRegular("server"_el)), 0);
        REQUIRE_EQUAL(namePath.find(Name::createRegular("worker"_el)), 1);
        REQUIRE_EQUAL(namePath.find(Name::createIndex(12)), 2);
        REQUIRE_EQUAL(namePath.find(Name::createText("worker"_el)), NamePath::npos);
        REQUIRE_EQUAL(namePath.find(Name::createRegular("value"_el)), NamePath::npos);

        namePath = {};
        REQUIRE_EQUAL(namePath.find(Name::createRegular("server"_el)), NamePath::npos);
    }

    void testSubPath() {
        namePath = NamePath{
            {Name::createRegular("a"_el),
                Name::createRegular("b"_el),
                Name::createRegular("c"_el),
                Name::createRegular("d"_el)}};

        REQUIRE_EQUAL(namePath.subPath(), namePath);
        REQUIRE_EQUAL(namePath.subPath(0, 0), NamePath{});
        REQUIRE_EQUAL(namePath.subPath(1, 2), NamePath::fromText("b.c"_el));
        REQUIRE_EQUAL(namePath.subPath(2), NamePath::fromText("c.d"_el));
        REQUIRE_EQUAL(namePath.subPath(2, NamePath::npos), NamePath::fromText("c.d"_el));
        REQUIRE_EQUAL(namePath.subPath(2, 100), NamePath::fromText("c.d"_el));
        REQUIRE_EQUAL(namePath.subPath(4), NamePath{});
        REQUIRE_EQUAL(namePath.subPath(5), NamePath{});

        namePath = {};
        REQUIRE_EQUAL(namePath.subPath(), NamePath{});
        REQUIRE_EQUAL(namePath.subPath(0), NamePath{});
        REQUIRE_EQUAL(namePath.subPath(1), NamePath{});
    }

    void testAppend() {
        // append individual elements.
        namePath = {};
        namePath.append({});
        REQUIRE_EQUAL(namePath.size(), 0);
        namePath.append(Name::createRegular("server"_el));
        REQUIRE_EQUAL(namePath.size(), 1);
        REQUIRE_EQUAL(namePath.at(0), Name::createRegular("server"_el));
        namePath.append(Name::createRegular("worker"_el));
        REQUIRE_EQUAL(namePath.size(), 2);
        REQUIRE_EQUAL(namePath.at(0), Name::createRegular("server"_el));
        REQUIRE_EQUAL(namePath.at(1), Name::createRegular("worker"_el));
        namePath.append({});
        REQUIRE_EQUAL(namePath.size(), 2);

        // append another path.
        namePath = NamePath{{Name::createRegular("server"_el), Name::createRegular("worker"_el)}};
        auto otherPath = NamePath{{Name::createRegular("value"_el), Name::createRegular("other"_el)}};
        namePath.append(otherPath);
        REQUIRE_EQUAL(namePath.size(), 4);
        REQUIRE_EQUAL(namePath.at(0), Name::createRegular("server"_el));
        REQUIRE_EQUAL(namePath.at(1), Name::createRegular("worker"_el));
        REQUIRE_EQUAL(namePath.at(2), Name::createRegular("value"_el));
        REQUIRE_EQUAL(namePath.at(3), Name::createRegular("other"_el));
    }

    void testPrepend() {
        // relative paths.
        namePath = NamePath{{Name::createRegular("server"_el), Name::createRegular("worker"_el)}};
        namePath.prepend({});
        REQUIRE_EQUAL(namePath.size(), 2);
        auto otherPath = NamePath{{Name::createRegular("value"_el), Name::createRegular("other"_el)}};
        namePath.prepend(otherPath);
        REQUIRE_EQUAL(namePath.size(), 4);
        REQUIRE_EQUAL(namePath.at(0), Name::createRegular("value"_el));
        REQUIRE_EQUAL(namePath.at(1), Name::createRegular("other"_el));
        REQUIRE_EQUAL(namePath.at(2), Name::createRegular("server"_el));
        REQUIRE_EQUAL(namePath.at(3), Name::createRegular("worker"_el));
    }

    void testHash() {
        namePath = {};
        constexpr auto hasher = std::hash<NamePath>();
        REQUIRE_EQUAL(hasher(namePath), hasher(NamePath{}));
        namePath = NamePath{{Name::createRegular("server"_el), Name::createRegular("worker"_el)}};
        const auto other = NamePath{{Name::createRegular("server"_el), Name::createRegular("worker"_el)}};
        REQUIRE_EQUAL(hasher(namePath), hasher(other));
    }

    void testFormat() {
        // As format uses toText(), no in-depth tests are required.
        namePath = NamePath{{Name::createRegular("server"_el), Name::createRegular("worker"_el)}};
        auto text = std::format("~{}~", namePath);
        REQUIRE_EQUAL(text, "~server.worker~");
    }

    void testInternalView() {
        namePath = NamePath{{Name::createRegular("server"_el), Name::createText("worker"_el), Name::createIndex(12)}};
        auto text = internalView(namePath)->toString();
        REQUIRE(text.contains("server"_el));
        REQUIRE(text.contains("worker"_el));
        REQUIRE(text.contains("12"_el));
        REQUIRE(text.contains("Regular"_el));
        REQUIRE(text.contains("Text"_el));
        REQUIRE(text.contains("Index"_el));
    }
};
