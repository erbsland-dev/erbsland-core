// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PathBackendTestBase.hpp"
#include "PathTestFixture.hpp"

#include <erbsland/ParseError.hpp>
#include <erbsland/path/impl/BackendFactory.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace el::text::literals;
using namespace erbsland::test::pathtest;

TESTED_TARGETS(PathWalker PathWalkOptions PathWalkFilterFn)
class PathWalkerFilterTest final : public el::UnitTest {
    class MetadataBackend final : public PathBackendTestBase {
    public:
        explicit MetadataBackend(el::Path root) : root{std::move(root)} {}

    public: // implement PathBackend
        auto loadInfoOrThrow(const el::Path &path, const el::PathInfoParts parts) const
            -> el::path::impl::PathInfoData override {
            if (parts.isSet(el::PathInfoPart::FileIdentity)) {
                metadataRequests.push_back(toStdString(path.name()));
            }
            auto data = el::path::impl::PathInfoData{};
            data.exists = true;
            data.type = path == root ? el::PathType::Directory : el::PathType::RegularFile;
            data.resolvedPath = path;
            data.loadedParts = parts | el::PathInfoPart::Type;
            return data;
        }
        auto directoryEntriesOrThrow(const el::Path &, const el::Path &) const -> std::vector<el::Path> override {
            auto result = std::vector<el::Path>{root / "keep.hpp"_el, root / "skip.txt"_el};
            for (auto &path : result) {
                preloadInfo(path, loadInfoOrThrow(path, el::PathInfoPart::Type));
            }
            return result;
        }

    public:
        el::Path root;
        mutable std::vector<std::string> metadataRequests;
    };

    class BackendScope final {
    public:
        explicit BackendScope(std::unique_ptr<MetadataBackend> backend) {
            el::path::impl::setPathBackend(std::move(backend));
        }
        ~BackendScope() { el::path::impl::setPathBackend(nullptr); }
    };

public:
    void testGlobPreservesLiteralFilenameComponents() {
        const auto fixture = PathTestFixture{"walker-glob-components"};
        // Backslash is a filename character on POSIX, but is forbidden on Windows.
#ifndef ERBSLAND_OS_WINDOWS
        const auto file = fixture.path() / el::Path::fromPosix(R"(a\b.cpp)"_el);
        file.content().writeTextOrThrow("a"_el);
        REQUIRE(file.info().isRegularFile());
        auto reported = std::vector<el::Path>{};
        const auto options = el::PathWalkOptions{}.setGlobPattern(R"(a\\b.cpp)"_el);
        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow(
                    [&](const el::Path &path) -> el::PathWalkStatus {
                        reported.push_back(path);
                        return el::PathWalkStatus::Continue;
                    },
                    options)
                .isSuccessful());
        REQUIRE_EQUAL(reported, std::vector<el::Path>{file});
#endif
    }

    void testGlobReportingPruningAndConvenienceSetter() {
        const auto fixture = PathTestFixture{"walker-globs"};
        std::filesystem::create_directories(fixture.stdPath() / "src" / "deep");
        std::filesystem::create_directories(fixture.stdPath() / "other");
        fixture.child("src/a.cpp").content().writeTextOrThrow("a"_el);
        fixture.child("src/deep/b.cpp").content().writeTextOrThrow("b"_el);
        fixture.child("other/c.cpp").content().writeTextOrThrow("c"_el);
        fixture.child("root.cpp").content().writeTextOrThrow("r"_el);
        for (const auto direction : {el::PathWalkDirection::RootToLeaf, el::PathWalkDirection::LeafToRoot}) {
            auto reported = std::vector<std::string>{};
            auto filtered = std::vector<std::string>{};
            auto descended = std::vector<std::string>{};
            auto options = el::PathWalkOptions{}
                               .setGlobPattern("src/*.cpp"_el)
                               .setDirection(direction)
                               .setTypes(el::PathType::RegularFile)
                               .setFilter([&](const el::Path &path, el::PathType) -> bool {
                                   filtered.push_back(toStdString(path.name()));
                                   return true;
                               })
                               .setDescendFilter([&](const el::Path &path, el::PathType) -> bool {
                                   descended.push_back(toStdString(path.name()));
                                   return true;
                               });
            const auto callback = [&](const el::Path &path) -> el::PathWalkStatus {
                reported.push_back(toStdString(path.name()));
                return el::PathWalkStatus::Continue;
            };
            REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
            REQUIRE_EQUAL(reported, std::vector<std::string>{"a.cpp"});
            REQUIRE_EQUAL(filtered, reported);
            REQUIRE_EQUAL(descended.size(), 2U);
            REQUIRE_EQUAL(descended.back(), "src");
            options.setGlobPattern("src/**"_el)
                .setTypes(el::PathTypes{el::PathType::RegularFile, el::PathType::Directory});
            reported.clear();
            filtered.clear();
            REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
            REQUIRE_EQUAL(filtered, (std::vector<std::string>{"src", "a.cpp", "deep", "b.cpp"}));
            if (direction == el::PathWalkDirection::RootToLeaf) {
                REQUIRE_EQUAL(reported, filtered);
            } else {
                REQUIRE_EQUAL(reported, (std::vector<std::string>{"a.cpp", "b.cpp", "deep", "src"}));
            }
            options.setTypes(el::PathType::RegularFile);
            options.setGlobMatchMode(el::PathGlobMatchMode::Suffix).setGlobPattern("*.cpp"_el);
            const auto previous = options.globPattern();
            REQUIRE_THROWS_AS(el::ParseError, options.setGlobPattern("bad["_el));
            REQUIRE(options.globPattern() == previous);
            reported.clear();
            REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
            REQUIRE_EQUAL(reported.size(), 4U);
            options.setGlobMatchMode(el::PathGlobMatchMode::Full);
            reported.clear();
            REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
            REQUIRE_EQUAL(reported, std::vector<std::string>{"root.cpp"});
            options.setGlobPattern("/"_el).setTypes(el::PathType::Directory);
            reported.clear();
            REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
            REQUIRE_EQUAL(reported, std::vector<std::string>{toStdString(fixture.path().name())});
        }
    }

    void testGlobPreservesCallbackControl() {
        const auto fixture = PathTestFixture{"walker-glob-control"};
        std::filesystem::create_directories(fixture.stdPath() / "src");
        fixture.child("src/file.cpp").content().writeTextOrThrow("a"_el);
        const auto options = el::PathWalkOptions{}.setGlobPattern("src/**"_el);
        for (const auto status : {el::PathWalkStatus::Skip, el::PathWalkStatus::Stop, el::PathWalkStatus::Failure}) {
            auto calls = 0;
            const auto result = fixture.path().walker().walkOrThrow(
                [&](const el::Path &path) -> el::PathWalkStatus {
                    ++calls;
                    REQUIRE_EQUAL(path.name(), "src"_el);
                    return status;
                },
                options);
            REQUIRE_EQUAL(calls, 1);
            if (status == el::PathWalkStatus::Skip) {
                REQUIRE_EQUAL(result, el::PathWalkResult::Success);
            } else if (status == el::PathWalkStatus::Stop) {
                REQUIRE_EQUAL(result, el::PathWalkResult::Stopped);
            } else {
                REQUIRE_EQUAL(result, el::PathWalkResult::Failure);
            }
        }
    }

    void testGlobPrunedAliasDoesNotConsumeDirectoryIdentity() {
        const auto fixture = PathTestFixture{"walker-glob-alias"};
        std::filesystem::create_directories(fixture.stdPath() / "a-target");
        fixture.child("a-target/file.cpp").content().writeTextOrThrow("a"_el);
        auto error = std::error_code{};
        std::filesystem::create_directory_symlink(fixture.stdPath() / "a-target", fixture.stdPath() / "z-alias", error);
        if (error) {
            return;
        }
        auto reported = std::vector<std::string>{};
        const auto options =
            el::PathWalkOptions{}.setGlobPattern("z-alias/*.cpp"_el).setTypes(el::PathType::RegularFile);
        REQUIRE(fixture.path()
                .walker()
                .walkOrThrow(
                    [&](const el::Path &path) -> el::PathWalkStatus {
                        reported.push_back(toStdString(path.parent().name()));
                        return el::PathWalkStatus::Continue;
                    },
                    options)
                .isSuccessful());
        REQUIRE_EQUAL(reported, std::vector<std::string>{"z-alias"});
    }

    void testGlobRejectedEntriesAvoidMetadataAndUserFilter() {
        const auto root = el::Path::systemTempDirectory() / "walker-glob-metadata"_el;
        auto backend = std::make_unique<MetadataBackend>(root);
        const auto *observed = backend.get();
        const auto scope = BackendScope{std::move(backend)};
        auto filterCalls = 0;
        const auto options = el::PathWalkOptions{}
                                 .setGlobPattern("*.hpp"_el)
                                 .setTypes(el::PathType::RegularFile)
                                 .setInfoParts(el::PathInfoPart::FileIdentity)
                                 .setFilter([&](const el::Path &, el::PathType) -> bool {
                                     ++filterCalls;
                                     return true;
                                 });
        REQUIRE(root.walker()
                .walkOrThrow(
                    [](const el::Path &) -> el::PathWalkStatus { return el::PathWalkStatus::Continue; }, options)
                .isSuccessful());
        REQUIRE_EQUAL(filterCalls, 1);
        REQUIRE_EQUAL(observed->metadataRequests, std::vector<std::string>{"keep.hpp"});
    }

    void testSymlinkPolicyPrecedesReportingFilter() {
        const auto fixture = PathTestFixture{"walker-filter-links"};
        fixture.child("target.txt").content().writeTextOrThrow("data"_el);
        auto error = std::error_code{};
        std::filesystem::create_symlink(
            fixture.child("target.txt").toStdPath(), fixture.child("link.txt").toStdPath(), error);
        if (error) {
            return; // Windows may lack symbolic-link privileges.
        }
        auto linkCalls = 0;
        auto linkType = el::PathType::Unknown;
        auto options = el::PathWalkOptions{}.setFilter([&](const el::Path &path, const el::PathType type) -> bool {
            if (path.name() == "link.txt"_el) {
                ++linkCalls;
                linkType = type;
            }
            return true;
        });
        const auto callback = [](const el::Path &) -> el::PathWalkStatus { return el::PathWalkStatus::Continue; };
        options.setSymlinkMode(el::SymlinkMode::Skip);
        REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
        REQUIRE_EQUAL(linkCalls, 0);
        options.setSymlinkMode(el::SymlinkMode::Follow);
        REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
        REQUIRE_EQUAL(linkCalls, 1);
        REQUIRE_EQUAL(linkType, el::PathType::RegularFile);
    }

    void testRejectedEntriesDoNotLoadRequestedMetadata() {
        const auto root = el::Path::systemTempDirectory() / "walker-filter-metadata"_el;
        auto backend = std::make_unique<MetadataBackend>(root);
        const auto *observedBackend = backend.get();
        const auto scope = BackendScope{std::move(backend)};
        auto visited = std::vector<std::string>{};
        const auto options =
            el::PathWalkOptions{}
                .setSymlinkMode(el::SymlinkMode::Skip)
                .setTypes(el::PathType::RegularFile)
                .setInfoParts(el::PathInfoPart::FileIdentity)
                .setFilter([](const el::Path &path, el::PathType) -> bool { return path.name().endsWith(".hpp"_el); });
        REQUIRE(root.walker()
                .walkOrThrow(
                    [&](const el::Path &path, const el::PathInfo &) -> el::PathWalkStatus {
                        visited.push_back(toStdString(path.name()));
                        return el::PathWalkStatus::Continue;
                    },
                    options)
                .isSuccessful());
        REQUIRE_EQUAL(visited, std::vector<std::string>{"keep.hpp"});
        REQUIRE_EQUAL(observedBackend->metadataRequests, std::vector<std::string>{"keep.hpp"});
    }

    void testReportingAndDescentAreIndependentInBothDirections() {
        const auto fixture = PathTestFixture{"walker-filters"};
        std::filesystem::create_directories(fixture.stdPath() / "a");
        std::filesystem::create_directories(fixture.stdPath() / "pruned");
        fixture.child("a/keep.hpp").content().writeTextOrThrow("keep"_el);
        fixture.child("a/skip.txt").content().writeTextOrThrow("skip"_el);
        fixture.child("pruned/hidden.hpp").content().writeTextOrThrow("hidden"_el);
        for (const auto direction : {el::PathWalkDirection::RootToLeaf, el::PathWalkDirection::LeafToRoot}) {
            auto visited = std::vector<std::string>{};
            auto filtered = std::vector<std::string>{};
            const auto options = el::PathWalkOptions{}
                                     .setDirection(direction)
                                     .setFilter([&](const el::Path &path, el::PathType) -> bool {
                                         filtered.push_back(toStdString(path.name()));
                                         return path.name().endsWith(".hpp"_el);
                                     })
                                     .setDescendFilter([](const el::Path &path, const el::PathType type) -> bool {
                                         return type == el::PathType::Directory && path.name() != "pruned"_el;
                                     });
            REQUIRE(fixture.path()
                    .walker()
                    .walkOrThrow(
                        [&](const el::Path &path) -> el::PathWalkStatus {
                            visited.push_back(toStdString(path.name()));
                            return el::PathWalkStatus::Continue;
                        },
                        options)
                    .isSuccessful());
            REQUIRE_EQUAL(visited, std::vector<std::string>{"keep.hpp"});
            REQUIRE_EQUAL(
                filtered,
                std::vector<std::string>(
                    {fixture.stdPath().filename().string(), "a", "keep.hpp", "skip.txt", "pruned"}));
        }
    }

    void testPrunedRootCanStillBeReportedAndFiltersCanBeCleared() {
        const auto fixture = PathTestFixture{"walker-filter-root"};
        fixture.child("child.txt").content().writeTextOrThrow("data"_el);
        auto count = 0;
        auto options =
            el::PathWalkOptions{}.setDescendFilter([](const el::Path &, el::PathType) -> bool { return false; });
        const auto callback = [&](const el::Path &) -> el::PathWalkStatus {
            ++count;
            return el::PathWalkStatus::Continue;
        };
        REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
        REQUIRE_EQUAL(count, 1);
        options.setDescendFilter({}).setFilter([](const el::Path &, el::PathType) -> bool { return false; });
        count = 0;
        REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
        REQUIRE_EQUAL(count, 0);
        options.setFilter({});
        REQUIRE(fixture.path().walker().walkOrThrow(callback, options).isSuccessful());
        REQUIRE_EQUAL(count, 2);
    }
};
