// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/github_workflows/WorkflowFile.hpp>

using namespace el::text::literals;

TESTED_TARGETS(tool::github_workflows::WorkflowFile)
class WorkflowFileTest final : public el::UnitTest {
public:
    void testPreservesQuotingCommentsMixedEndingsAndFinalNewlineState() {
        const auto original = "steps:\r\n  # owner/repo@v1.0.0\r\n  - uses: 'owner/repo@old' # keep\r\n"
                              "  - uses: owner/repo/sub@old\n  - uses: ./local\n  - uses: docker://image:tag"_el;
        auto file = tool::github_workflows::WorkflowFile{el::Path{"ci.yml"_el}, original};
        REQUIRE_EQUAL(file.text(), original);
        REQUIRE_EQUAL(file.references().count(), el::ItemCount{2});
        const auto first = file.references().first();
        REQUIRE(file.hasAnnotation(first, "v1.0.0"_el));
        file.repair(first, "new"_el, "v2.0.0"_el);
        file.repair(file.references().last(), "next"_el, "v3.0.0"_el);
        REQUIRE_EQUAL(
            file.text(),
            "steps:\r\n  # owner/repo@v2.0.0\r\n  - uses: 'owner/repo@new' # keep\r\n"
            "  # owner/repo/sub@v3.0.0\n  - uses: owner/repo/sub@next\n  - uses: ./local\n  - uses: docker://image:tag"_el);
    }

    void testIgnoresCommentsAndBlockScalars() {
        const auto text = "# uses: wrong/repo@bad\nsteps:\n  - run: |\n      uses: ignored/repo@bad\n"
                          "      # text\n\n  - run: >-\n      uses: ignored/repo@bad\n"
                          "  - uses: owner/repo/path@old\n"_el;
        const auto file = tool::github_workflows::WorkflowFile{el::Path{"ci.yml"_el}, text};
        REQUIRE_EQUAL(file.references().count(), el::ItemCount{1});
        REQUIRE_EQUAL(file.references().first().repository, "owner/repo"_el);
        REQUIRE_EQUAL(file.references().first().line, el::ItemIndex{8});
        REQUIRE_EQUAL(file.text(), text);
    }

    void testRejectsUnsupportedValuesAndRecognizesUnpinnedActions() {
        for (
            const auto &text : el::StringList{
                "uses: |\n  owner/repo@sha\n"_el,
                "uses: ${{ matrix.action }}\n"_el,
                "- { uses: owner/repo@sha }\n"_el,
                "steps: [{name: 'uses: ignored/repo@sha', uses: owner/repo@sha}]\n"_el,
                "\"uses\": owner/repo@sha\n"_el,
                "uses: 'owner/repo@sha\n"_el,
                "uses: 'owner/repo@sha' garbage\n"_el}) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    REQUIRE_THROWS_AS(
                        el::ApplicationError, tool::github_workflows::WorkflowFile(el::Path{"ci.yml"_el}, text));
                },
                [&]() -> std::string { return el::StringConverter{text}.toStdString(); });
        }
        const auto file = tool::github_workflows::WorkflowFile{el::Path{"ci.yml"_el}, "uses: owner/repo\n"_el};
        REQUIRE(file.references().first().ref.isEmpty());
        REQUIRE_FALSE(tool::github_workflows::WorkflowFile::isAction("owner/../repo"_el));
        REQUIRE(tool::github_workflows::WorkflowFile::isSha("abcdef0123456789abcdef0123456789abcdef01"_el));
        REQUIRE_FALSE(tool::github_workflows::WorkflowFile::isSha("abc"_el));
    }

    void testIgnoresUsesInTextAndRequiresWhitespaceBeforeComments() {
        const auto text = "name: 'uses: ignored/repo@sha'\n- run: echo uses: ignored/repo@sha\n"
                          "steps: [{name: '{ uses: ignored/repo@sha }'}]\n"
                          "uses: owner/repo@aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa#suffix # keep\n"_el;
        auto file = tool::github_workflows::WorkflowFile{el::Path{"ci.yml"_el}, text};
        REQUIRE_EQUAL(file.references().count(), el::ItemCount{1});
        const auto reference = file.references().first();
        REQUIRE_FALSE(tool::github_workflows::WorkflowFile::isSha(reference.ref));
        file.repair(reference, "new"_el, "v1.0.0"_el);
        REQUIRE(file.text().endsWith("uses: owner/repo@new # keep\n"_el));
    }
};
