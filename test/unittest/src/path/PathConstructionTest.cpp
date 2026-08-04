// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestHelper.hpp"

#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <string>
#include <vector>

using el::path::Path;
using el::path::PathFormat;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;
namespace th = erbsland::unittest::th;

TESTED_TARGETS(Path)
class PathConstructionTest final : public el::UnitTest {
public:
    void testGenericConstruction() {
        struct TestCase {
            el::text::String input;
            std::string expected;
            bool absolute;
            PathFormat format;
        };
        const auto testCases = std::vector<TestCase>{
            {"mission//logs/"_el, "mission/logs", false, PathFormat::Generic},
            {"mission\\logs\\dia.txt"_el, "mission/logs/dia.txt", false, PathFormat::Generic},
            {"mission//logs\\dia.txt"_el, "mission/logs/dia.txt", false, PathFormat::Generic},
            {"\\arquivo\\rotas"_el, "arquivo/rotas", false, PathFormat::Generic},
            {"C:relative"_el, "C:relative", false, PathFormat::Generic},
            {"./atlas/../relatorio"_el, "./atlas/../relatorio", false, PathFormat::Generic},
            {"/var//skyship/log.txt"_el, "/var/skyship/log.txt", true, PathFormat::Posix},
            {"C:\\Expedicao\\Plano.txt"_el, "c:/Expedicao/Plano.txt", true, PathFormat::Windows},
            {"//PORTO/Partilha/Mapa.txt"_el, "//porto/Partilha/Mapa.txt", true, PathFormat::Windows},
            {"\\\\PORTO\\Partilha\\Mapa.txt"_el, "//porto/Partilha/Mapa.txt", true, PathFormat::Windows},
        };

        for (const auto &[input, expected, absolute, format] : testCases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    const auto path = Path{input};
                    REQUIRE(path.isValid());
                    REQUIRE_EQUAL(path.isAbsolute(), absolute);
                    REQUIRE_EQUAL(path.format(), format);
                    REQUIRE_EQUAL(toStdString(path), expected);
                },
                [&]() -> std::string { return std::format("input: {}", toStdString(input)); });
        }
    }

    void testFormatSpecificConstruction() {
        REQUIRE_EQUAL(toStdString(Path::fromPosix("c:/diario/voo"_el)), "c:/diario/voo");
        REQUIRE_EQUAL(Path::fromPosix("c:/diario/voo"_el).format(), PathFormat::Posix);
        REQUIRE(Path::fromPosix("c:/diario/voo"_el).isRelative());

        REQUIRE_EQUAL(toStdString(Path::fromWindows("C:\\Diario\\voo.txt"_el)), "c:/Diario/voo.txt");
        REQUIRE_EQUAL(toStdString(Path::fromWindows("diario/voo\\mapa.txt"_el)), "diario/voo/mapa.txt");
        REQUIRE_EQUAL(Path::fromWindows("diario/voo\\mapa.txt"_el).format(), PathFormat::Windows);
        REQUIRE_EQUAL(toStdString(Path::fromWindows("\\arquivo\\rotas"_el)), "/arquivo/rotas");
        REQUIRE_EQUAL(Path::fromWindows("\\arquivo\\rotas"_el).format(), PathFormat::Posix);
        REQUIRE_EQUAL(toStdString(Path::fromWindows("\\\\?\\C:\\Diario\\voo.txt"_el)), "c:/Diario/voo.txt");
        REQUIRE_EQUAL(
            toStdString(Path::fromWindows("\\\\?\\UNC\\PORTO\\Partilha\\mapa.txt"_el)), "//porto/Partilha/mapa.txt");

        REQUIRE_EQUAL(toStdString(Path::fromPosix("arquivo\\rotas/voo.txt"_el)), "arquivo\\rotas/voo.txt");
        REQUIRE_EQUAL(Path::fromPosix("arquivo\\rotas/voo.txt"_el).format(), PathFormat::Posix);
    }

    void testInvalidConstruction() {
        REQUIRE(Path{}.isEmpty());
        REQUIRE_FALSE(Path{}.isValid());
        REQUIRE(Path{""_el}.isEmpty());
        REQUIRE(Path::fromWindows("C:relative"_el).isEmpty());
        REQUIRE(Path{"//server"_el}.isEmpty());
        REQUIRE(Path{"//?/Volume{123}/"_el}.isEmpty());
        REQUIRE(Path{"/?\?/C:/secret"_el}.isEmpty());
        REQUIRE(Path::fromWindows("//?/Volume{123}/"_el).isEmpty());
        REQUIRE(Path::fromWindows("//./PhysicalDrive0"_el).isEmpty());
        REQUIRE(Path::fromWindows("\\??\\C:\\secret"_el).isEmpty());
        REQUIRE(Path::fromWindows("//server"_el).isEmpty());

        auto nulText = std::string{"mapa"};
        nulText.push_back('\0');
        nulText += "oculto";
        REQUIRE(Path{el::text::String{nulText}}.isEmpty());

        const auto invalidUtf8 = th::stdStringFromHex("61 C0 80 62");
        REQUIRE(Path{el::text::String{invalidUtf8}}.isEmpty());

        auto nulElementText = std::string{"visivel"};
        nulElementText.push_back('\0');
        nulElementText += "oculto";
        const auto nulElement = el::text::String{nulElementText};
        REQUIRE(Path::fromElements(el::text::StringList{"arquivo"_el, nulElement}).isEmpty());

        const auto invalidUtf8Element = el::text::StringEditor{th::stdStringFromHex("61 C0 80 62")};
        REQUIRE(Path::fromElements(el::text::StringList{"arquivo"_el, invalidUtf8Element}).isEmpty());
    }

    void testHardLimits() {
        REQUIRE_FALSE(Path{el::text::StringEditor{std::string(8193U, 'a')}}.isValid());

        auto tooManyElementsText = std::string{};
        for (auto index = 0; index < 1001; ++index) {
            if (!tooManyElementsText.empty()) {
                tooManyElementsText += '/';
            }
            tooManyElementsText += 'a';
        }
        REQUIRE(Path{el::text::StringEditor{tooManyElementsText}}.isEmpty());

        auto tooManyElements = el::text::StringList{};
        for (auto index = 0; index < 1001; ++index) {
            tooManyElements.append("a"_el);
        }
        REQUIRE(Path::fromElements(tooManyElements).isEmpty());

        const auto longElement = el::text::StringEditor{std::string(4096U, 'a')};
        REQUIRE(Path::fromElements(el::text::StringList{longElement, longElement}).isEmpty());
    }
};
