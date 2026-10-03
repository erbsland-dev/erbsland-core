// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/StringJoins.hpp>

TESTED_TARGETS(ProjectRoot)
class ProjectRootTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testAncestorDiscovery() {
        const auto nested = _root / "a/b/c/d"_el;
        nested.operations().createDirectoryOrThrow(el::PathCreateDirectoryOptions{}.setCreateParents(true));
        REQUIRE_EQUAL(tool::frame::ProjectRoot{nested}.discover(), _root);
    }

    void testCharacterBoundary() {
        write(
            "CMakeLists.txt"_el,
            el::join(el::String::fromCharacter(U'\n', el::CpCount{1002}), "project(erbsland-core)\n"_el));
        REQUIRE_EQUAL(tool::frame::ProjectRoot{_root}.validate(), _root);
        write(
            "CMakeLists.txt"_el,
            el::join(el::String::fromCharacter(U'\n', el::CpCount{1024}), "project(erbsland-core)\n"_el));
        REQUIRE_THROWS(tool::frame::ProjectRoot{_root}.validate());
    }

    void testMarkerWhitespaceAndCase() {
        write("CMakeLists.txt"_el, "  PrOjEcT ( erbsland-core LANGUAGES CXX)\n"_el);
        REQUIRE_EQUAL(tool::frame::ProjectRoot{_root}.validate(), _root);
        write("CMakeLists.txt"_el, "project(erbsland-core)\n"_el);
        REQUIRE_EQUAL(tool::frame::ProjectRoot{_root}.validate(), _root);
    }

    void testInvalidMarkers() {
        write("CMakeLists.txt"_el, "# project(erbsland-core)\nproject(erbsland-core-tool)\n"_el);
        REQUIRE_THROWS(tool::frame::ProjectRoot{_root}.validate());
        REQUIRE_THROWS(tool::frame::ProjectRoot{_root}.discover());
    }
};
