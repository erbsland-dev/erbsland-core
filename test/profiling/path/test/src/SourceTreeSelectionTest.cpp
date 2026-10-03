// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../src/impl/SourceTreeSelection.hpp"

#include <erbsland/Path.hpp>
#include <erbsland/PathType.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/SymlinkMode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;
namespace selection = app::path::impl;

TESTED_TARGETS(SourceTreeSelection)
class SourceTreeSelectionTest final : public el::UnitTest {
public:
    void testRegularSourceFiles() {
        for (const auto &name : el::StringList{"unit.hpp"_el, "unit.cpp"_el, "unit.tpp"_el}) {
            REQUIRE(selection::isSourceFile(el::Path{name}, el::PathType::RegularFile));
            REQUIRE_FALSE(selection::isSourceFile(el::Path{name}, el::PathType::Directory));
            REQUIRE_FALSE(selection::isSourceFile(el::Path{name}, el::PathType::Symlink));
        }
        for (const auto &name : el::StringList{"unit.hpp.in"_el, "unit.cpp.o"_el, "unit.txt"_el, "unit.CPP"_el}) {
            REQUIRE_FALSE(selection::isSourceFile(el::Path{name}, el::PathType::RegularFile));
        }
    }

    void testDirectoryPruning() {
        const auto options = selection::sourceTreeWalkOptions();
        REQUIRE(options.symlinkMode() == el::SymlinkMode::Skip);
        for (
            const auto &name : el::StringList{
                ".git"_el,
                ".cache"_el,
                "__pycache__"_el,
                "CMakeFiles"_el,
                "target"_el,
                "build"_el,
                "_build"_el,
                "cmake-build-debug"_el,
                "cmake-build-release"_el}) {
            const auto path = el::Path{"source"_el} / name;
            REQUIRE_FALSE(selection::shouldDescendSourceTree(path));
            REQUIRE_FALSE(options.descendFilter()(path, el::PathType::Directory));
            REQUIRE_FALSE(options.filter()(path, el::PathType::Directory));
        }
        for (const auto &name : el::StringList{"src"_el, "impl"_el, "generated"_el, "builder"_el, "targets"_el}) {
            const auto path = el::Path{name};
            REQUIRE(selection::shouldDescendSourceTree(path));
            REQUIRE(options.descendFilter()(path, el::PathType::Directory));
            REQUIRE(options.filter()(path, el::PathType::Directory));
        }
    }
};
