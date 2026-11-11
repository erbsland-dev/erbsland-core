// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../src/GitStatusReport.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(GitStatusReport)
class GitStatusReportTest final : public el::UnitTest {
public:
    void testEmptyAndIgnored() {
        auto report = demo::GitStatusReport{};
        report.append({});
        report.finish();
        REQUIRE(report.directories().count().isZero());
        report.append("!! ignored\0"_el);
        report.finish();
        REQUIRE(report.directories().count().isZero());
    }
    void testCategoriesAndDirectories() {
        auto report = demo::GitStatusReport{};
        report.append("M  src/a\0 M src/b\0MM src/c\0?? doc/new/file\0A  README\0 D gone\0"_el);
        report.finish();
        REQUIRE_EQUAL(report.directories().count(), el::ItemCount{3});
        const auto source = report.directories().get("src/"_el).value();
        REQUIRE_EQUAL(source.staged, 2U);
        REQUIRE_EQUAL(source.unstaged, 2U);
        REQUIRE_EQUAL(source.untracked, 0U);
        REQUIRE_EQUAL(source.conflicted, 0U);
        REQUIRE_EQUAL(report.directories().get("doc/"_el)->untracked, 1U);
        REQUIRE_EQUAL(report.directories().get("(root)"_el)->staged, 1U);
        REQUIRE_EQUAL(report.directories().get("(root)"_el)->unstaged, 1U);
        REQUIRE_EQUAL(report.directories().first().first, "(root)"_el);
    }
    void testDirectoryNamedLikeRootLabel() {
        auto report = demo::GitStatusReport{};
        report.append("?? root-file\0?? (root)/file\0"_el);
        report.finish();
        REQUIRE_EQUAL(report.directories().count(), el::ItemCount{2});
        REQUIRE_EQUAL(report.directories().get("(root)"_el)->untracked, 1U);
        REQUIRE_EQUAL(report.directories().get("(root)/"_el)->untracked, 1U);
    }
    void testAllConflictForms() {
        auto report = demo::GitStatusReport{};
        report.append("DD d/a\0AU d/b\0UD d/c\0UA d/d\0DU d/e\0AA d/f\0UU d/g\0"_el);
        report.finish();
        const auto counts = report.directories().get("d/"_el).value();
        REQUIRE_EQUAL(counts.conflicted, 7U);
        REQUIRE_EQUAL(counts.staged, 0U);
        REQUIRE_EQUAL(counts.unstaged, 0U);
    }
    void testRenamesCopiesAndTypeChanges() {
        auto report = demo::GitStatusReport{};
        report.append("R  new/a\0old/a\0CM new/b\0old/b\0 T new/c\0"_el);
        report.finish();
        REQUIRE_EQUAL(report.directories().count(), el::ItemCount{1});
        const auto counts = report.directories().get("new/"_el).value();
        REQUIRE_EQUAL(counts.staged, 2U);
        REQUIRE_EQUAL(counts.unstaged, 2U);
    }
    void testUnusualNamesAndEveryChunkBoundary() {
        const auto data = el::String{"?? études/a b\n\t.txt\0R  new/place\0old/é\0MM root file\0"_el};
        for (auto split = el::ByteIndex{}; split <= el::ByteIndex{} + data.length(); ++split) {
            auto report = demo::GitStatusReport{};
            report.append(data.slice(el::ByteRange{el::ByteIndex{}, split}));
            report.append({});
            report.append(data.slice(el::ByteRange{split, el::ByteCount::infinite()}));
            report.finish();
            REQUIRE_EQUAL(report.directories().get("études/"_el)->untracked, 1U);
            REQUIRE_EQUAL(report.directories().get("new/"_el)->staged, 1U);
            REQUIRE_EQUAL(report.directories().get("(root)"_el)->unstaged, 1U);
        }
        auto report = demo::GitStatusReport{};
        for (auto index = el::ByteIndex{}; index.isWithin(data.length()); ++index) {
            report.append(data.slice(el::ByteRange{index, el::ByteCount{1}}));
        }
        report.finish();
        REQUIRE_EQUAL(report.directories().count(), el::ItemCount{3});
    }
    void testIncompleteRecords() {
        auto report = demo::GitStatusReport{};
        report.append(" M src/file"_el);
        REQUIRE_THROWS_AS(el::ApplicationError, report.finish());
        report.append("\0R  new/file\0"_el);
        REQUIRE_THROWS_AS(el::ApplicationError, report.finish());
        report.append("old/file"_el);
        REQUIRE_THROWS_AS(el::ApplicationError, report.finish());
        report.append("\0"_el);
        report.finish();
    }
    void testMalformedRecords() {
        for (
            const auto &record :
            {"\0"_el, " M \0"_el, "ZZ file\0"_el, "MMxfile\0"_el, "  file\0"_el, "R  new/file\0\0"_el}) {
            auto report = demo::GitStatusReport{};
            REQUIRE_THROWS_AS(el::ApplicationError, report.append(record));
        }
    }
};
