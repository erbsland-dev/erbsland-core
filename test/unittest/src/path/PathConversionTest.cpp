// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestHelper.hpp"

#include <erbsland/err/ParseError.hpp>
#include <erbsland/path/PathWindowsFormat.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using el::path::Path;
using el::path::PathWindowsFormat;
using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path)
class PathConversionTest final : public el::UnitTest {
public:
    void testStringConversions() {
        const auto relative = Path{"diario/voo.txt"_el};
        REQUIRE_EQUAL(toStdString(relative.toString()), "diario/voo.txt");
        REQUIRE_EQUAL(toStdString(relative.toPosix()), "diario/voo.txt");
        REQUIRE_EQUAL(toStdString(relative.toWindows(PathWindowsFormat::Native)), "diario\\voo.txt");

        const auto windows = Path::fromWindows("C:\\Diario\\voo.txt"_el);
        REQUIRE(windows.toPosix().isEmpty());
        REQUIRE_EQUAL(toStdString(windows.toWindows(PathWindowsFormat::Native)), "c:\\Diario\\voo.txt");
        REQUIRE_EQUAL(toStdString(windows.toWindows(PathWindowsFormat::Extended)), "\\\\?\\c:\\Diario\\voo.txt");

        const auto unc = Path::fromWindows("\\\\PORTO\\Partilha\\voo.txt"_el);
        REQUIRE_EQUAL(toStdString(unc.toWindows(PathWindowsFormat::Native)), "\\\\porto\\Partilha\\voo.txt");
        REQUIRE_EQUAL(toStdString(unc.toWindows(PathWindowsFormat::Extended)), "\\\\?\\UNC\\porto\\Partilha\\voo.txt");

        const auto posix = Path::fromPosix("/arquivo/voo.txt"_el);
        REQUIRE(posix.toWindows().isEmpty());
    }

    void testStdFilesystemInterop() {
        const auto path = Path{std::filesystem::path{"arquivo/rotas/voo.txt"}};
        REQUIRE_EQUAL(toStdString(path), "arquivo/rotas/voo.txt");
        const auto genericPath = path.toStdPath().generic_string();
        REQUIRE_EQUAL(genericPath, "arquivo/rotas/voo.txt");
    }

    void testThrowingFactories() {
        REQUIRE_NOTHROW(Path::fromPosixOrThrow("/arquivo"_el));
        REQUIRE_NOTHROW(Path::fromWindowsOrThrow("C:\\Arquivo"_el));
        REQUIRE_THROWS_AS(el::err::ParseError, Path::fromWindowsOrThrow("C:relative"_el));
        REQUIRE_THROWS_AS(el::err::ParseError, Path::fromNativeOrThrow({}));
    }
};
