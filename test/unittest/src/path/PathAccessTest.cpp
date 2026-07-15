// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestHelper.hpp"

#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ElementIndex.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using el::path::Path;
using el::unit::ElementCount;
using el::unit::ElementIndex;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path)
class PathAccessTest final : public el::UnitTest {
public:
    void testElementsAndRoots() {
        const auto path = Path{"//PORTO/Partilha/rotas/manha.txt"_el};
        REQUIRE(path.isAbsolute());
        REQUIRE_FALSE(path.isRoot());
        REQUIRE_EQUAL(path.elementCount(), ElementCount{3});
        REQUIRE_EQUAL(toStdString(path.root()), "//porto/Partilha/");
        REQUIRE_EQUAL(toStdString(path.element(ElementIndex{0})), "//porto/Partilha/");
        REQUIRE_EQUAL(toStdString(path.element(ElementIndex{1})), "rotas");
        REQUIRE_EQUAL(toStdString(path.element(ElementIndex{2})), "manha.txt");
        REQUIRE(path.element(ElementIndex{3}).isEmpty());
        REQUIRE_EQUAL(path.elements().count(), ElementCount{3});
    }

    void testParents() {
        const auto path = Path{"/arquivo/rotas/manha.txt"_el};
        REQUIRE_EQUAL(toStdString(path.parent()), "/arquivo/rotas");

        const auto parents = path.parents();
        REQUIRE_EQUAL(parents.count(), ElementCount{3});
        REQUIRE_EQUAL(toStdString(parents.get(ElementIndex{0})), "/arquivo/rotas");
        REQUIRE_EQUAL(toStdString(parents.get(ElementIndex{1})), "/arquivo");
        REQUIRE_EQUAL(toStdString(parents.get(ElementIndex{2})), "/");
        REQUIRE(Path{"/"_el}.isRoot());
        REQUIRE(Path{"/"_el}.parent().isEmpty());
    }

    void testNameAndSuffixes() {
        const auto report = Path{"relatorios/ceu.claro.final.txt"_el};
        REQUIRE_EQUAL(toStdString(report.name()), "ceu.claro.final.txt");
        REQUIRE_EQUAL(toStdString(report.suffix()), ".txt");
        REQUIRE_EQUAL(toStdString(report.suffixes()), ".claro.final.txt");
        REQUIRE_EQUAL(toStdString(report.stem()), "ceu");

        const auto hidden = Path{".perfil"_el};
        REQUIRE(hidden.suffix().isEmpty());
        REQUIRE(hidden.suffixes().isEmpty());
        REQUIRE_EQUAL(toStdString(hidden.stem()), ".perfil");

        const auto trailingDot = Path{"relatorio."_el};
        REQUIRE_EQUAL(toStdString(trailingDot.suffix()), ".");
        REQUIRE_EQUAL(toStdString(trailingDot.suffixes()), ".");
        REQUIRE_EQUAL(toStdString(trailingDot.stem()), "relatorio");
        REQUIRE(Path{"/"_el}.name().isEmpty());
    }

    void testComparison() {
        REQUIRE(Path{"A/B"_el}.compare(Path{"a/b"_el}) < 0);
        REQUIRE_EQUAL(
            Path{"A/B"_el}.compare(Path{"a/b"_el}, el::text::Char::compareCaseFolded), std::strong_ordering::equal);
    }
};
