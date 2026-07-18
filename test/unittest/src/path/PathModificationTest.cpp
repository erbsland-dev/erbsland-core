// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestHelper.hpp"

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/unit/ElementRange.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>

using el::path::Path;
using el::unit::ElementCount;
using el::unit::ElementIndex;
using el::unit::ElementRange;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path)
class PathModificationTest final : public el::UnitTest {
public:
    void testNameTools() {
        const auto path = Path{"diario/voo.final.txt"_el};
        REQUIRE_EQUAL(toStdString(path.withName("mapa.dat"_el)), "diario/mapa.dat");
        REQUIRE_EQUAL(toStdString(path.withSuffix("md"_el)), "diario/voo.md");
        REQUIRE_EQUAL(toStdString(path.withSuffix(".rst"_el)), "diario/voo.rst");
        REQUIRE_EQUAL(toStdString(path.withSuffix({})), "diario/voo");
        REQUIRE_EQUAL(toStdString(path.withStem("rota"_el)), "diario/rota.final.txt");
        REQUIRE(Path{"/"_el}.withName("x"_el).isEmpty());
    }

    void testNameToolsRejectNullCharacters() {
        const auto path = Path{"diario/voo.final.txt"_el};
        auto text = std::string{"visivel"};
        text.push_back('\0');
        text += "oculto";
        const auto replacement = el::text::StringEditor{text};

        REQUIRE(path.withName(replacement).isEmpty());
        REQUIRE(path.withSuffix(replacement).isEmpty());
        REQUIRE(path.withStem(replacement).isEmpty());
    }

    void testJoin() {
        const auto base = Path{"/arquivo/rotas"_el};
        REQUIRE_EQUAL(toStdString(base / "manha.txt"_el), "/arquivo/rotas/manha.txt");
        REQUIRE_EQUAL(toStdString(base / Path{"/absoluto/ignorado.txt"_el}), "/arquivo/rotas/absoluto/ignorado.txt");

        auto mutablePath = Path{"diario"_el};
        mutablePath /= "expedicao"_el;
        mutablePath /= Path{"dia1.txt"_el};
        REQUIRE_EQUAL(toStdString(mutablePath), "diario/expedicao/dia1.txt");
    }

    void testSlice() {
        const auto path = Path{"/arquivo/rotas/manha.txt"_el};
        REQUIRE_EQUAL(toStdString(path.slice(ElementRange{ElementIndex{0}, ElementCount{2}})), "/arquivo");
        REQUIRE_EQUAL(toStdString(path.slice(ElementRange{ElementIndex{1}, ElementCount{2}})), "arquivo/rotas");
        REQUIRE_EQUAL(
            toStdString(path.slice(ElementRange{ElementIndex{1}, ElementCount::infinite()})),
            "arquivo/rotas/manha.txt");
        REQUIRE_EQUAL(toStdString(path.slice(ElementRange{ElementIndex{2}, ElementCount{20}})), "rotas/manha.txt");
        REQUIRE(path.slice(ElementRange{ElementIndex{4}, ElementCount{1}}).isEmpty());
    }

    void testSplitAfter() {
        const auto path = Path{"/arquivo/rotas/manha.txt"_el};

        auto [emptyFront, wholeBack] = path.splitAfter(ElementCount::zero());
        REQUIRE(emptyFront.isEmpty());
        REQUIRE_EQUAL(toStdString(wholeBack), "/arquivo/rotas/manha.txt");

        auto [rootFront, rootBack] = path.splitAfter(ElementCount{1});
        REQUIRE_EQUAL(toStdString(rootFront), "/");
        REQUIRE_EQUAL(toStdString(rootBack), "arquivo/rotas/manha.txt");

        auto [archiveFront, archiveBack] = path.splitAfter(ElementCount{2});
        REQUIRE_EQUAL(toStdString(archiveFront), "/arquivo");
        REQUIRE_EQUAL(toStdString(archiveBack), "rotas/manha.txt");

        auto [directoryFront, directoryBack] = path.splitAfter(ElementCount{3});
        REQUIRE_EQUAL(toStdString(directoryFront), "/arquivo/rotas");
        REQUIRE_EQUAL(toStdString(directoryBack), "manha.txt");

        auto [wholeFront, currentBack] = path.splitAfter(ElementCount{4});
        REQUIRE_EQUAL(toStdString(wholeFront), "/arquivo/rotas/manha.txt");
        REQUIRE_EQUAL(toStdString(currentBack), ".");

        auto [infiniteFront, infiniteBack] = path.splitAfter(ElementCount::infinite());
        REQUIRE_EQUAL(toStdString(infiniteFront), "/arquivo/rotas/manha.txt");
        REQUIRE_EQUAL(toStdString(infiniteBack), ".");

        auto [invalidFront, invalidBack] = Path{}.splitAfter(ElementCount{1});
        REQUIRE(invalidFront.isEmpty());
        REQUIRE(invalidBack.isEmpty());
    }

    void testSpecialElements() {
        const auto &empty = Path::empty();
        REQUIRE(empty.isEmpty());
        REQUIRE_FALSE(empty.isValid());
        REQUIRE(&empty == &Path::empty());

        const auto current = Path::currentElement();
        REQUIRE(current.isValid());
        REQUIRE(current.isRelative());
        REQUIRE_EQUAL(toStdString(current), ".");

        const auto parent = Path::parentElement();
        REQUIRE(parent.isValid());
        REQUIRE(parent.isRelative());
        REQUIRE_EQUAL(toStdString(parent), "..");
    }

    void testFromElements() {
        const auto path = Path::fromElements(el::text::StringList{"/"_el, "arquivo"_el, "rotas"_el});
        REQUIRE(path.isAbsolute());
        REQUIRE_EQUAL(toStdString(path), "/arquivo/rotas");

        const auto relative = Path::fromElements(el::text::StringList{"arquivo"_el, "rotas"_el});
        REQUIRE(relative.isRelative());
        REQUIRE_EQUAL(toStdString(relative), "arquivo/rotas");
    }
};
