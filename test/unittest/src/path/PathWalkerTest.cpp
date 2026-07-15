// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathTestFixture.hpp"

#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/path/PathWalkDirection.hpp>
#include <erbsland/path/PathWalker.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <filesystem>
#include <string>
#include <vector>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(Path PathWalker PathWalkOptions PathWalkResult)
class PathWalkerTest final : public el::UnitTest {
public:
    void testEmptyAndInvalidCallback() {
        const auto walker = el::path::PathWalker{};
        REQUIRE(walker.isEmpty());
        REQUIRE(walker.path().isEmpty());
        const auto callback = el::path::PathWalkFn{};
        REQUIRE(walker.walk(callback).isFailure());
        REQUIRE_THROWS_AS(el::path::PathError, walker.walkOrThrow(callback));
    }

    void testTraversalOrderAndBase() {
        const auto fixture = PathTestFixture{"walker-order"};
        std::filesystem::create_directories(fixture.stdPath() / "a");
        fixture.child("a/one.txt").content().writeTextOrThrow("one"_el);
        fixture.child("b.txt").content().writeTextOrThrow("two"_el);

        auto visited = std::vector<std::string>{};
        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow([&](const el::path::Path &path) -> el::path::PathWalkStatus {
                    visited.push_back(toStdString(path.name()));
                    return el::path::PathWalkStatus::Continue;
                })
                .isSuccessful());
        REQUIRE_EQUAL(
            visited, std::vector<std::string>({fixture.stdPath().filename().string(), "a", "one.txt", "b.txt"}));

        visited.clear();
        auto options = el::path::PathWalkOptions{};
        options.setDirection(el::path::PathWalkDirection::LeafToRoot);
        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow(
                    [&](const el::path::Path &path) -> el::path::PathWalkStatus {
                        visited.push_back(toStdString(path.name()));
                        return el::path::PathWalkStatus::Continue;
                    },
                    options)
                .isSuccessful());
        REQUIRE_EQUAL(
            visited, std::vector<std::string>({"one.txt", "a", "b.txt", fixture.stdPath().filename().string()}));
    }

    void testCallbackControlAndTypeFilter() {
        const auto fixture = PathTestFixture{"walker-control"};
        std::filesystem::create_directories(fixture.stdPath() / "skip");
        fixture.child("skip/hidden.txt").content().writeTextOrThrow("hidden"_el);
        fixture.child("visible.txt").content().writeTextOrThrow("visible"_el);

        auto visited = std::vector<std::string>{};
        auto options = el::path::PathWalkOptions{};
        options.setTypes(el::path::PathType::RegularFile);
        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow(
                    [&](const el::path::Path &path) -> el::path::PathWalkStatus {
                        visited.push_back(toStdString(path.name()));
                        return el::path::PathWalkStatus::Continue;
                    },
                    options)
                .isSuccessful());
        REQUIRE_EQUAL(visited, std::vector<std::string>({"hidden.txt", "visible.txt"}));

        visited.clear();
        options.setTypes(el::path::PathType::All);
        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow(
                    [&](const el::path::Path &path) -> el::path::PathWalkStatus {
                        visited.push_back(toStdString(path.name()));
                        return path.name() == "skip"_el ? el::path::PathWalkStatus::Skip
                                                        : el::path::PathWalkStatus::Continue;
                    },
                    options)
                .isSuccessful());
        REQUIRE_EQUAL(
            visited, std::vector<std::string>({fixture.stdPath().filename().string(), "skip", "visible.txt"}));

        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow(
                    [](const el::path::Path &) -> el::path::PathWalkStatus { return el::path::PathWalkStatus::Stop; })
                .isSuccessful());
        REQUIRE_EQUAL(
            fixture.path().walker().walkOrThrow(
                [](const el::path::Path &) -> el::path::PathWalkStatus { return el::path::PathWalkStatus::Failure; }),
            el::path::PathWalkResult::Failure);
    }

    void testIgnoredFilesystemErrorReturnsFailure() {
        const auto fixture = PathTestFixture{"walker-errors"};
        auto options = el::path::PathWalkOptions{};
        options.setIgnoreErrors(true);
        const auto result = fixture.child("missing").walker().walkOrThrow(
            [](const el::path::Path &) -> el::path::PathWalkStatus { return el::path::PathWalkStatus::Continue; },
            options);
        REQUIRE(result.isFailure());
    }
};
