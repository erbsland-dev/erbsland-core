// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

#include <erbsland/re/StdFormat.hpp>
#include <erbsland/unittest/FileHelper.hpp>

#include <vector>

using namespace el::re;
using namespace el::unittest::fh;
namespace string_helper = re_test::string_helper;

TESTED_TARGETS(RegEx)
TAGS(RealWorld Performance)
class RegExRealWorldTest final : public UNITTEST_SUBCLASS(RegExBase) {
    std::unique_ptr<String> _shakespeareText = nullptr;
    std::unique_ptr<String> _shakespeareHtml = nullptr;
    std::unique_ptr<String> _shakespeareHtmlOnce = nullptr;

    [[nodiscard]] static auto loadText(const std::string_view filename) -> std::unique_ptr<String> {
        auto offset = el::unit::ByteIndex::zero();
        auto text = String{readDataText(filename)};
        if (text.startsWith(string_helper::bytesToString({0xEF, 0xBB, 0xBF}))) {
            offset = el::unit::ByteIndex{3U};
        }
        return std::make_unique<String>(text.slice(el::text::StringSide::Back, offset));
    }

    // create test text by repeating the loaded text twice.
    static auto loadAndRepeatText(const std::string_view filename) -> std::unique_ptr<String> {
        auto offset = el::unit::ByteIndex::zero();
        auto text = String{readDataText(filename)};
        if (text.startsWith(string_helper::bytesToString({0xEF, 0xBB, 0xBF}))) {
            offset = el::unit::ByteIndex{3U};
        }
        const auto sourceText = text.slice(el::text::StringSide::Back, offset);
        constexpr auto repetitionCount = 2U;
        const auto finalSize = sourceText.length() * repetitionCount;
        auto result = StringEditor{};
        result.reserve(finalSize);
        for (std::size_t i = 0; i < repetitionCount; ++i) {
            result.append(sourceText);
        }
        return std::make_unique<String>(result);
    }

    auto shakespeare() -> const String & {
        if (!_shakespeareText) {
            _shakespeareText = loadAndRepeatText("data/re/shakespeare.txt");
        }
        return *_shakespeareText;
    }

    auto shakespeareHtml() -> const String & {
        if (!_shakespeareHtml) {
            _shakespeareHtml = loadAndRepeatText("data/re/shakespeare.html");
        }
        return *_shakespeareHtml;
    }

    auto shakespeareHtmlOnce() -> const String & {
        if (!_shakespeareHtmlOnce) {
            _shakespeareHtmlOnce = loadText("data/re/shakespeare.html");
        }
        return *_shakespeareHtmlOnce;
    }

    auto countMatchesIn(const String &text) -> std::size_t {
        this->text = String{text};
        REQUIRE(regex);
        std::size_t matchCount = 0;
        for (const auto &match : regex->findAll(text)) {
            REQUIRE(match);
            matchCount += 1;
        }
        return matchCount;
    }

    auto extractGroupsToLines(
        const String &text, const String &pattern, const Flags flags, const std::vector<std::size_t> &groupIndices)
        -> std::vector<std::string> {

        requireCompile(pattern, flags);
        this->text = String{text};
        REQUIRE(regex);

        auto lines = std::vector<std::string>{};
        for (const auto &match : regex->findAll(text)) {
            REQUIRE(match);
            lastMatch = match;

            auto line = std::string{};
            for (std::size_t i = 0; i < groupIndices.size(); ++i) {
                const auto groupIndex = groupIndices[i];
                const auto groupCount = match->groupCount();
                REQUIRE_LESS(groupIndex, groupCount);
                if (i != 0) {
                    line += "|";
                }
                line += string_helper::toStdString(match->content(groupIndex));
            }
            lines.emplace_back(std::move(line));
        }

        return lines;
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAllWords() {
        requireCompile(R"(\b\w+\b)"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 229192);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCapitalizedWords() {
        requireCompile(R"(\b[A-Z][a-z]*\b)"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 38760);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testEmailAddresses() {
        requireCompile(R"(([a-zA-Z0-9\._%\+\-]+)@([a-zA-Z0-9\.\-]+\.[a-zA-Z]{2,}))"_el);
        // Sanity test: Make sure the pattern works.
        REQUIRE_EQUAL(countMatchesIn("012 unit-test@example.com 345 unit-test@example.com 678"_el), 2);
        // There are no email addresses in the test text, so no matches must be found.
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 0);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testURLs() {
        requireCompile(R"(https?://([a-zA-Z0-9\.]+))"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 2);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testHtmlTags() {
        requireCompile(R"(<[a-z1-6]+[^>]*>)"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeareHtml()), 27588);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testExtractTocLinksCaptureGroups() {
        const auto pattern = R"re(<a href="#(chap([0-9]{2}))" class="pginternal">([^<]+)</a>)re"_el;
        const auto groupIndices = std::vector<std::size_t>{1, 2, 3};

        const auto actualLines = extractGroupsToLines(shakespeareHtmlOnce(), pattern, {}, groupIndices);
        const auto expectedLines = readDataLines("data/re/toc_links.txt");
        requireLines(actualLines, expectedLines);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testExtractLicenseDivCaptureGroups() {
        const auto pattern = R"re(<div id="(([^-\"]+)-([^-\"]+)-([^"]+))">([^<]+)</div>)re"_el;
        const auto groupIndices = std::vector<std::size_t>{1, 2, 3, 4, 5};

        const auto actualLines = extractGroupsToLines(shakespeareHtmlOnce(), pattern, {}, groupIndices);
        const auto expectedLines = readDataLines("data/re/license_div.txt");
        requireLines(actualLines, expectedLines);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testExtractTocLinksPossessiveQuantifiersCaptureGroups() {
        const auto pattern = R"re(<a href=\"#(chap([0-9]{2}))\" class=\"pginternal\">([^<]++)</a>)re"_el;
        const auto groupIndices = std::vector<std::size_t>{1, 2, 3};

        const auto actualLines = extractGroupsToLines(shakespeareHtmlOnce(), pattern, {}, groupIndices);
        const auto expectedLines = readDataLines("data/re/toc_links_possessive.txt");
        requireLines(actualLines, expectedLines);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testDotPlus() {
        requireCompile(R"(.+)"_el, Flag::CRLF);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 34916);

        requireCompile(R"((?s).+)"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 1);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSimpleWord() {
        requireCompile(R"(simple)"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 32);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSimpleWordAtStart() {
        requireCompile(R"((?m)^This)"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 150);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSimpleWordInMiddle() {
        requireCompile(R"(contains)"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 2);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSimpleWordWithBoundary() {
        requireCompile(R"(\bsimple\b)"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 28);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testComplexMarkdownLinks() {
        requireCompile(R"(\[([^\]]+)\]\(([^\)]+)\))"_el);
        REQUIRE_EQUAL(countMatchesIn(shakespeare()), 0);
    }
};
