// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/ItemCount.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringCharReader.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringSplitter.hpp>
#include <tool/idna/LabelProcessor.hpp>
#include <tool/psl/PublicSuffixList.hpp>
#include <tool/psl/PublicSuffixTool.hpp>
#include <tool/unicode/UcdReader.hpp>

#include <optional>

TESTED_TARGETS(PublicSuffixList PublicSuffixTool LabelProcessor)
class PublicSuffixFullTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testOfficialIdnaConformance() {
        _root = tool::frame::ProjectRoot{el::Path::currentDirectory()}.discover();
        runWithSources(el::StringList{tool::unicode::UnicodeSource::sourceIdentifier()}, [&] {
            const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
            const auto dataset = tool::unicode::UcdDataset{
                source.directory(), source.version(), el::StringList{"Idna2008.txt"_el, "IdnaTestV2.txt"_el}};
            const auto &attributes = source.idnaData();
            const auto processor = tool::idna::LabelProcessor{attributes, source.normalizer(), source.mapping()};
            for (
                const auto &record : tool::unicode::UcdReader{dataset.text("Idna2008.txt"_el), "Idna2008.txt"_el}.parse(
                    el::ItemCount{2}, true)) {
                const auto status = record.fields.get(el::ItemIndex{1});
                auto expectedStatus = 0U;
                if (status == "PVALID"_el) {
                    expectedStatus = 1U;
                } else if (status == "CONTEXTJ"_el) {
                    expectedStatus = 2U;
                } else if (status == "CONTEXTO"_el || status == "CONTEXT0"_el) {
                    expectedStatus = 3U;
                } else {
                    REQUIRE(status == "DISALLOWED"_el || status == "UNASSIGNED"_el);
                }
                for (auto cp = record.first; cp <= record.last; ++cp) {
                    REQUIRE_EQUAL(attributes.attributes(cp) & 3U, expectedStatus);
                }
            }
            verifyOfficialMappingTests(dataset.text("IdnaTestV2.txt"_el), processor);
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCompleteEncodingAndDeterminism() {
        _root = tool::frame::ProjectRoot{el::Path::currentDirectory()}.discover();
        runWithSources(
            el::StringList{
                tool::unicode::UnicodeSource::sourceIdentifier(), tool::psl::PublicSuffixSource::sourceIdentifier()},
            [&] {
                const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
                const auto &snapshot = tool::frame::app().source<tool::psl::PublicSuffixSource>();
                const auto processor =
                    tool::idna::LabelProcessor{source.idnaData(), source.normalizer(), source.mapping()};
                const auto parsed =
                    tool::psl::PublicSuffixList{snapshot.text(), snapshot.inputFile().toString(), processor};
                const auto data = tool::psl::EncodedData{parsed.rules()};
                const auto again = tool::psl::EncodedData{parsed.rules()};
                data.verify();
                REQUIRE_EQUAL(data.encodedRules(), again.encodedRules());
                REQUIRE_EQUAL(data.prefixTree(), again.prefixTree());
                REQUIRE_EQUAL(data.contentTree(), again.contentTree());
                REQUIRE_EQUAL(data.blockOffsets(), again.blockOffsets());
                REQUIRE(data.storageByteCount() <= el::ByteCount{45000});
                REQUIRE_EQUAL(parsed.version(), snapshot.version());
            });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCurrentOutputFreshness() {
        _root = tool::frame::ProjectRoot{el::Path::currentDirectory()}.discover();
        runWithSources(
            el::StringList{
                tool::unicode::UnicodeSource::sourceIdentifier(), tool::psl::PublicSuffixSource::sourceIdentifier()},
            [&] {
                const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
                const auto &snapshot = tool::frame::app().source<tool::psl::PublicSuffixSource>();
                const auto processor =
                    tool::idna::LabelProcessor{source.idnaData(), source.normalizer(), source.mapping()};
                const auto parsed =
                    tool::psl::PublicSuffixList{snapshot.text(), snapshot.inputFile().toString(), processor};
                const auto data = tool::psl::EncodedData{parsed.rules()};
                const auto output = tool::psl::PublicSuffixTool::render(parsed.version(), data);
                REQUIRE_EQUAL(output, tool::psl::PublicSuffixTool::render(parsed.version(), data));
                REQUIRE_FALSE(
                    files().updateText(
                        el::Path{"src/erbsland/network/impl/http/cookie/PublicSuffixData.cpp"_el}, output, true));
            });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRuleSyntaxAndLabelPreparation() {
        _root = tool::frame::ProjectRoot{el::Path::currentDirectory()}.discover();
        runWithSources(el::StringList{tool::unicode::UnicodeSource::sourceIdentifier()}, [&] {
            const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
            const auto processor = tool::idna::LabelProcessor{source.idnaData(), source.normalizer(), source.mapping()};
            REQUIRE_EQUAL(processor.toAscii("faß.de"_el), "xn--fa-hia.de"_el);
            REQUIRE_EQUAL(processor.toAscii("ＦＯＯ.de"_el), "foo.de"_el);
            REQUIRE_EQUAL(processor.toAscii("XN--BCHER-KVA.de"_el), "xn--bcher-kva.de"_el);
            for (
                const auto &bad : el::StringList{
                    "a..de"_el,
                    "-a"_el,
                    "ab--cd"_el,
                    "xn--abc-"_el,
                    "́a"_el,
                    "a_b"_el,
                    "a·b"_el,
                    "aא"_el,
                    "a\u200Db"_el}) {
                REQUIRE_THROWS(processor.toAscii(bad));
            }
            const auto snapshot = el::String{
                "// VERSION: test\n// ===BEGIN ICANN DOMAINS===\nfoo\nfoo\n*.bar\n!baz\n// ===END ICANN DOMAINS===\n// ===BEGIN PRIVATE DOMAINS===\n// ===END PRIVATE DOMAINS===\n"_el};
            REQUIRE_EQUAL(
                (tool::psl::PublicSuffixList{snapshot, "fixture"_el, processor}.rules().count().toSizeT()), 3U);
            REQUIRE_THROWS(
                tool::psl::PublicSuffixList{snapshot.replacedAll("*.bar"_el, "*.foo"_el), "fixture"_el, processor});
            REQUIRE_THROWS(
                tool::psl::PublicSuffixList{snapshot.replacedAll("!baz"_el, "! baz"_el), "fixture"_el, processor});
            REQUIRE_THROWS(
                tool::psl::PublicSuffixList{
                    snapshot.replacedAll("// ===END PRIVATE DOMAINS==="_el, ""_el), "fixture"_el, processor});
        });
    }

private:
    void verifyOfficialMappingTests(const el::String &text, const tool::idna::LabelProcessor &processor) {
        auto lines = el::StringSplitter{text, U'\n'};
        auto checked = 0U;
        while (!lines.isAtEnd()) {
            auto line = lines.next();
            const auto comment = line.find("#"_el);
            if (comment.isValid()) {
                line = line.slice(el::StringSide::Front, comment);
            }
            line = line.trimmed(el::CharSet{U' ', U'\t', U'\r'});
            if (line.isEmpty()) {
                continue;
            }
            auto fields = el::StringList{};
            auto split = el::StringSplitter{line, U';'};
            while (!split.isAtEnd()) {
                fields.append(split.next().trimmed(el::CharSet{U' ', U'\t'}));
            }
            REQUIRE(fields.count() >= el::ItemCount{5U});
            auto status = fields.get(el::ItemIndex{4});
            if (status.isEmpty()) {
                status = fields.get(el::ItemIndex{2});
            }
            if (!status.isEmpty() && status != "[]"_el) {
                const auto invalid = unescape(fields.get(el::ItemIndex{0}));
                // Full-domain bidi checks differ from the historical per-label PSL profile.
                if (invalid && !invalid->findFirstOf(el::CharSet{U'.', U'。', U'．', U'｡'}).isValid()) {
                    REQUIRE_THROWS(processor.toAscii(*invalid));
                }
                continue;
            }
            const auto source = unescape(fields.get(el::ItemIndex{0}));
            auto unicode = source;
            if (!fields.get(el::ItemIndex{1}).isEmpty()) {
                unicode = unescape(fields.get(el::ItemIndex{1}));
            }
            auto expected = unicode;
            if (!fields.get(el::ItemIndex{3}).isEmpty()) {
                expected = unescape(fields.get(el::ItemIndex{3}));
            }
            if (!source || !unicode || !expected) {
                continue;
            }
            // UTS #46 permits additional symbols; select cases also valid under our strict IDNA2008 profile.
            try {
                [[maybe_unused]] const auto prepared = processor.toAscii(*unicode);
            } catch (const el::RuntimeError &) {
                continue;
            }
            REQUIRE_EQUAL(processor.toAscii(*source), *expected);
            ++checked;
        }
        REQUIRE(checked > 300U);
    }

    [[nodiscard]] static auto unescape(const el::String &text) -> std::optional<el::String> {
        if (text == "\"\""_el) {
            return el::String{};
        }
        auto reader = el::StringCharReader{text};
        auto result = el::StringEditor{};
        while (!reader.isAtEnd()) {
            auto character = reader.read();
            if (character == U'\\') {
                if (reader.isAtEnd()) {
                    return {};
                }
                const auto type = reader.read();
                auto digits = 4U;
                if (type == U'U') {
                    digits = 8U;
                } else if (type != U'u') {
                    return {};
                }
                auto code = el::StringEditor{};
                for (auto index = 0U; index < digits; ++index) {
                    if (reader.isAtEnd()) {
                        return {};
                    }
                    code.append(reader.read());
                }
                const auto value = code.toIntegerOrThrow<unsigned>(el::IntegerParseOptions{}
                        .setFixedBase(el::IntegerBase::Hexadecimal)
                        .setMaximumDigits(el::CpCount{8})
                        .setFlags(el::IntegerParseFlags{}));
                if (value > 0x10FFFFU || (value >= 0xD800U && value <= 0xDFFFU)) {
                    return {};
                }
                character = el::Char{static_cast<char32_t>(value)};
            }
            result.append(character);
        }
        return el::String{result};
    }
};
