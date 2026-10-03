// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <tool/frame/ProjectTextFile.hpp>

using namespace el::text::literals;

TESTED_TARGETS(ProjectFiles ProjectTextFile ProjectAccessCheck)
class ProjectFilesTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
private:
    static constexpr auto cAlphaValue = "alpha.value"_el;
    static constexpr auto cDataValue = "data.value"_el;

public:
    void testCreateDirectoriesPreservesExistingContentsAndConfinesPaths() {
        const auto service = files();
        service.createDirectories(el::Path{"new/nested"_el});
        REQUIRE((_root / "new/nested"_el).info().isDirectory());
        write("new/nested/value.txt"_el, "keep"_el);
        service.createDirectories(_root / "new/nested"_el);
        REQUIRE_EQUAL(service.readText(el::Path{"new/nested/value.txt"_el}), "keep"_el);
        REQUIRE_THROWS(service.createDirectories(el::Path{"../outside"_el}));
        REQUIRE_THROWS(service.createDirectories(el::Path{"new/nested/value.txt/child"_el}));
    }

    void testCollectFilenamesOnlyFiltersSortsAndControlsRecursion() {
        write("inputs/z.hpp"_el, "z"_el);
        write("inputs/a.hpp"_el, "a"_el);
        write("inputs/skip.cpp"_el, "skip"_el);
        write("inputs/nested/b.hpp"_el, "b"_el);
        const auto service = files();
        const auto top = service.collectFilenamesOnly(el::Path{"inputs"_el}, ".hpp"_el, false);
        REQUIRE_EQUAL(top.count(), el::ItemCount{2});
        REQUIRE_EQUAL(top.getRef(el::ItemIndex{0}), _root / "inputs/a.hpp"_el);
        REQUIRE_EQUAL(top.getRef(el::ItemIndex{1}), _root / "inputs/z.hpp"_el);
        const auto all = service.collectFilenamesOnly(_root / "inputs"_el, ".hpp"_el);
        REQUIRE_EQUAL(all.count(), el::ItemCount{3});
        REQUIRE_EQUAL(all.getRef(el::ItemIndex{1}), _root / "inputs/nested/b.hpp"_el);
        const auto single = service.collectFilenamesOnly(el::Path{"inputs/a.hpp"_el}, ".hpp"_el);
        REQUIRE_EQUAL(single.count(), el::ItemCount{1});
        REQUIRE_THROWS(service.collectFilenamesOnly(el::Path{"inputs/skip.cpp"_el}, ".hpp"_el));
        REQUIRE_THROWS(service.collectFilenamesOnly(el::Path{"missing"_el}, ".hpp"_el));
        REQUIRE_THROWS(service.collectFilenamesOnly(el::Path{"../outside"_el}, ".hpp"_el));
    }

    void testTolerantTextReadsPreserveMalformedPayload() {
        const auto path = _root / "tolerant.txt"_el;
        path.content().writeDataOrThrow(el::ByteBlock{0xefU, 0xbbU, 0xbfU, 0x61U, 0xffU, 0x0aU});
        const auto expected = el::String{el::unittest::th::stdStringFromHex("61ff0a")};
        const auto service = files();
        REQUIRE(service.readText(path).compareBinary(expected) == std::strong_ordering::equal);
        REQUIRE(service.readTextFile(path).text().compareBinary(expected) == std::strong_ordering::equal);
        auto found = false;
        REQUIRE(_root.walker()
                .walkOrThrow(
                    [&](const el::Path &entry, const el::PathInfo &) -> el::PathWalkStatus {
                        if (entry == path) {
                            const auto snapshot = service.readWalkedTextFile(entry);
                            REQUIRE(snapshot.text().compareBinary(expected) == std::strong_ordering::equal);
                            found = true;
                        }
                        return el::PathWalkStatus::Continue;
                    },
                    el::PathWalkOptions{}.setTypes(el::PathType::RegularFile).setSymlinkMode(el::SymlinkMode::Skip))
                .isSuccessful());
        REQUIRE(found);
    }

    void testImmediateWalkReadsRemainProjectBound() {
        write("data/value.txt"_el, "contents"_el);
        (_temporary->path() / "outside.txt"_el).content().writeTextOrThrow("outside"_el);
        auto error = std::error_code{};
        std::filesystem::create_symlink(
            (_temporary->path() / "outside.txt"_el).toStdPath(), (_root / "data/linked.txt"_el).toStdPath(), error);
        const auto service = files();
        auto readCount = 0;
        REQUIRE((_root / "data"_el)
                .walker()
                .walkOrThrow(
                    [&](const el::Path &path, const el::PathInfo &) -> el::PathWalkStatus {
                        const auto file = service.readWalkedTextFile(path);
                        REQUIRE_EQUAL(file.text(), "contents"_el);
                        REQUIRE(service.isUnchanged(file, "contents"_el));
                        ++readCount;
                        return el::PathWalkStatus::Continue;
                    },
                    el::PathWalkOptions{}.setTypes(el::PathType::RegularFile).setSymlinkMode(el::SymlinkMode::Skip))
                .isSuccessful());
        REQUIRE_EQUAL(readCount, 1);
        REQUIRE_THROWS(service.readWalkedTextFile(el::Path{"../outside.txt"_el}));
        REQUIRE_THROWS(service.readWalkedTextFile(_root / "data"_el));
    }

    void testContainmentAndTraversal() {
        const auto service = files();
        write("inside.txt"_el, "inside"_el);
        REQUIRE_EQUAL(service.readText(el::Path{"inside.txt"_el}), "inside"_el);
        REQUIRE_THROWS(service.readText(el::Path{"../outside.txt"_el}));
        REQUIRE_THROWS(service.checked(_temporary->path() / "project-other/file"_el));
        REQUIRE_THROWS(service.checked(el::Path{}));
    }

    void testCheckedNativeComponents() {
#ifndef ERBSLAND_OS_WINDOWS
        const auto relative = el::Path::fromNative(R"(a\b/c\d.txt)"_el);
        const auto path = _root / relative;
        path.content().writeTextOrThrow("old"_el, el::PathWriteTextOptions{}.setCreateParents(true));
        const auto service = files();
        REQUIRE_EQUAL(service.checked(relative), path);
        REQUIRE_EQUAL(service.checked(path), path);
        REQUIRE(service.updateText(relative, "new"_el));
        REQUIRE_EQUAL(path.content().readTextOrThrow(), "new"_el);
#endif
    }

    void testFileErrorContext() {
        for (
            const auto &path :
            el::List<el::Path>{el::Path{}, el::Path{"missing.txt"_el}, el::Path{"../outside.txt"_el}}) {
            try {
                [[maybe_unused]] const auto text = files().readText(path);
                REQUIRE(false);
            } catch (const el::ApplicationError &error) {
                REQUIRE_FALSE(error.title().isEmpty());
                REQUIRE_FALSE(error.description().isEmpty());
                REQUIRE_FALSE(error.sourcePath().isEmpty());
                REQUIRE_FALSE(error.cause());
            }
        }
    }

    void testConfinedIncludesAndWildcards() {
        write("conf/main.elcl"_el, "@include: \"tools/*.elcl\"\n"_el);
        write("conf/tools/b.elcl"_el, "[beta]\nvalue: 2\n"_el);
        write("conf/tools/a.elcl"_el, "[alpha]\nvalue: 1\n"_el);
        const auto configuration = files().readConfiguration(el::Path{"conf/main.elcl"_el});
        REQUIRE_EQUAL(configuration->valueOrThrow(cAlphaValue)->asIntegerOrThrow(), 1);
        REQUIRE_EQUAL(configuration->valueOrThrow("beta.value"_el)->asIntegerOrThrow(), 2);
        write("conf/main.elcl"_el, "@include: \"t*/a.elcl\"\n"_el);
        const auto directoryGlob = files().readConfiguration(el::Path{"conf/main.elcl"_el});
        REQUIRE_EQUAL(directoryGlob->valueOrThrow(cAlphaValue)->asIntegerOrThrow(), 1);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRejectedParentAndRecursiveIncludes() {
        (_temporary->path() / "outside.elcl"_el).content().writeTextOrThrow("[outside]\nvalue: 1\n"_el);
        write("main.elcl"_el, "@include: \"../*.elcl\"\n"_el);
        REQUIRE_THROWS(files().readConfiguration(el::Path{"main.elcl"_el}));
        write("main.elcl"_el, "@include: \"**/*.elcl\"\n"_el);
        REQUIRE_THROWS(files().readConfiguration(el::Path{"main.elcl"_el}));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testUpdateAndCheckMode() {
        const auto service = files();
        write("output.txt"_el, "old"_el);
        REQUIRE(service.updateText(el::Path{"output.txt"_el}, "new"_el, true));
        REQUIRE_EQUAL(service.readText(el::Path{"output.txt"_el}), "old"_el);
        REQUIRE(service.updateText(el::Path{"output.txt"_el}, "new"_el));
        const auto modified = (_root / "output.txt"_el).info().lastModified();
        REQUIRE_FALSE(service.updateText(el::Path{"output.txt"_el}, "new"_el));
        REQUIRE_EQUAL((_root / "output.txt"_el).info().lastModified(), modified);
        REQUIRE_THROWS(service.updateText(el::Path{"missing/output.txt"_el}, "new"_el));
        REQUIRE_EQUAL(service.readText(el::Path{"output.txt"_el}), "new"_el);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testEquivalentGeneratedLines() {
        for (
            const auto &[existing, proposed] : el::List<std::pair<el::String, el::String>>{
                {"a\r\nb\r\n"_el, "a\nb\n"_el},
                {"a\nb"_el, "a\r\nb\n"_el},
                {"a\r\nb\nc\r\n"_el, "a\nb\r\nc"_el},
                {""_el, ""_el},
                {"// Copyright (c) 2025 Owner\r\nbody\r\n"_el, "// Copyright (c) 2026 Owner\nbody\n"_el},
                {"// Copyright © 2020-2025 Owner\n"_el, "// Copyright © 2020-2026 Owner"_el},
                {"# Copyright (c) 2025 Owner"_el, "# Copyright (c) 2026 Owner\n"_el}}) {
            write("output.txt"_el, existing);
            const auto path = _root / "output.txt"_el;
            const auto modified = path.info().lastModified();
            for (const auto check : {true, false}) {
                REQUIRE_FALSE(files().updateText(path, proposed, check));
                REQUIRE_EQUAL(files().readText(path), existing);
                REQUIRE_EQUAL(path.info().lastModified(), modified);
            }
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMeaningfulGeneratedChanges() {
        for (
            const auto &[existing, proposed] : el::List<std::pair<el::String, el::String>>{
                {""_el, "\n"_el},
                {"a\n"_el, "a\n\n"_el},
                {" a"_el, "a"_el},
                {"a\t"_el, "a "_el},
                {"// Copyright © 2025 A"_el, "// Copyright © 2026 B"_el},
                {"// Copyright © 2025 A"_el, "// Copyright (c) 2026 A"_el},
                {"header\n// Copyright © 2025 A\n"_el, "header\n// Copyright © 2026 A\n"_el},
                {"// Copyright © 2025 A\nold"_el, "// Copyright © 2026 A\nnew"_el}}) {
            write("output.txt"_el, existing);
            REQUIRE(files().updateText(el::Path{"output.txt"_el}, proposed, true));
            REQUIRE_EQUAL(files().readText(el::Path{"output.txt"_el}), existing);
            REQUIRE(files().updateText(el::Path{"output.txt"_el}, proposed));
            REQUIRE_EQUAL(files().readText(el::Path{"output.txt"_el}), proposed);
        }
    }

    void testMissingOutputAncestorsAndFileParents() {
        const auto service = files();
        const auto nested = el::Path{"missing/deep/output.txt"_el};
        REQUIRE_EQUAL(service.checked(nested, true), _root / nested);
        REQUIRE_THROWS(service.checked(nested));
        write("file.txt"_el, "contents"_el);
        REQUIRE_THROWS(service.checked(el::Path{"file.txt/child/output.txt"_el}, true));
        REQUIRE_THROWS(service.checked(el::Path{"../../outside.txt"_el}, true));
    }

    void testTextSnapshotDetectsEditsRemovalAndReplacement() {
        const auto service = files();
        REQUIRE_FALSE(service.isUnchanged(tool::frame::ProjectTextFile{}, ""_el));
        write("output.txt"_el, "old"_el);
        const auto file = service.readTextFile(el::Path{"output.txt"_el});
        REQUIRE_EQUAL(file.text(), "old"_el);
        REQUIRE(service.isUnchanged(file, "old"_el));
        REQUIRE_FALSE(service.isUnchanged(file, "new"_el));
        write("output.txt"_el, "edited"_el);
        REQUIRE_FALSE(service.isUnchanged(file, "old"_el));
        const auto replacement = service.readTextFile(el::Path{"output.txt"_el});
        const auto timestamp = std::filesystem::last_write_time(replacement.path().toStdPath());
        replacement.path().operations().removeOrThrow();
        REQUIRE_FALSE(service.isUnchanged(replacement, "edited"_el));
        write("output.txt"_el, "other!"_el);
        std::filesystem::last_write_time(replacement.path().toStdPath(), timestamp);
        REQUIRE_FALSE(service.isUnchanged(replacement, "edited"_el));
        REQUIRE(service.updateText(replacement.path(), "edited"_el));
        REQUIRE_EQUAL(service.readText(replacement.path()), "edited"_el);
    }

    void testTextSnapshotPreservesEquivalentLineEndings() {
        const auto service = files();
        write("output.txt"_el, "// Copyright (c) 2025 Owner\r\nbody\r\n"_el);
        const auto file = service.readTextFile(el::Path{"output.txt"_el});
        REQUIRE(service.isUnchanged(file, "// Copyright (c) 2026 Owner\nbody\n"_el));
        REQUIRE_EQUAL(service.readText(file.path()), file.text());
    }

    void testConfinedReadsAndSnapshotsRejectLinkedParents() {
        write("original/output.txt"_el, "contents"_el);
        const auto service = files();
        const auto file = service.readTextFile(el::Path{"original/output.txt"_el});
        auto error = std::error_code{};
        std::filesystem::create_directory_symlink(
            (_root / "original"_el).toStdPath(), (_root / "linked"_el).toStdPath(), error);
        if (error) {
            return; // Windows may lack symbolic-link privileges.
        }
        REQUIRE_THROWS(service.readText(el::Path{"linked/output.txt"_el}));
        REQUIRE_THROWS(service.readData(el::Path{"linked/output.txt"_el}));
        REQUIRE_THROWS(service.readTextFile(el::Path{"linked/output.txt"_el}));
        REQUIRE_THROWS(service.checked(el::Path{"linked/new/output.txt"_el}, true));
        REQUIRE_THROWS(service.createDirectories(el::Path{"linked/new"_el}));
        REQUIRE_THROWS(service.collectFilenamesOnly(el::Path{"linked"_el}, ".txt"_el));
        const auto names = service.collectFilenamesOnly(el::Path{"."_el}, ".txt"_el);
        REQUIRE(names.contains(_root / "original/output.txt"_el));
        REQUIRE_FALSE(names.contains(_root / "linked/output.txt"_el));
        std::filesystem::rename((_root / "original"_el).toStdPath(), (_root / "moved"_el).toStdPath());
        std::filesystem::create_directory_symlink(
            (_root / "moved"_el).toStdPath(), (_root / "original"_el).toStdPath());
        REQUIRE_FALSE(service.isUnchanged(file, "contents"_el));
        REQUIRE_THROWS(service.updateText(file.path(), "new"_el));
        REQUIRE_EQUAL(service.readText(el::Path{"moved/output.txt"_el}), "contents"_el);
    }

    void testManifestPlaceholdersRemainLiteral() {
        write("manifest.elcl"_el, "[data]\nvalue: \"${var:year}\"\n"_el);
        REQUIRE_EQUAL(
            files().readConfiguration(el::Path{"manifest.elcl"_el}, false)->getTextOrThrow(cDataValue),
            "${var:year}"_el);
        REQUIRE_EQUAL(
            files()
                .readConfiguration(
                    el::Path{"manifest.elcl"_el}, true, el::StringMap<el::String>{{"year"_el, "2030"_el}})
                ->getTextOrThrow(cDataValue),
            "2030"_el);
    }
};
