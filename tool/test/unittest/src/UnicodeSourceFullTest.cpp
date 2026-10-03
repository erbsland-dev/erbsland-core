// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/StringEditor.hpp>
#include <erbsland/StringSplitter.hpp>
#include <tool/unicode/NfcNormalizer.hpp>
#include <tool/unicode/UcdReader.hpp>

TESTED_TARGETS(NfcNormalizer UnicodeData)
class UnicodeSourceFullTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testOfficialNfcConformance() {
        _root = tool::frame::ProjectRoot{el::Path::currentDirectory()}.discover();
        runWithSources(el::StringList{tool::unicode::UnicodeSource::sourceIdentifier()}, [&] {
            const auto &source = tool::frame::app().source<tool::unicode::UnicodeSource>();
            const auto dataset = tool::unicode::UcdDataset{
                source.directory(), source.version(), el::StringList{"NormalizationTest.txt"_el}};
            const auto &normalizer = source.normalizer();
            auto lines = el::StringSplitter{dataset.text("NormalizationTest.txt"_el), U'\n'};
            auto count = 0U;
            while (!lines.isAtEnd()) {
                auto line = lines.next();
                const auto comment = line.find("#"_el);
                if (comment.isValid()) {
                    line = line.slice(el::StringSide::Front, comment);
                }
                line = line.trimmed(el::CharSet{U' ', U'\t', U'\r'});
                if (line.isEmpty() || line.startsWith("@"_el)) {
                    continue;
                }
                auto fields = el::List<el::String>{};
                auto split = el::StringSplitter{line, U';'};
                for (auto index = el::ItemIndex{0U}; index.isWithin(el::ItemCount{5U}); index.uncheckedIncrement()) {
                    fields.append(fromCodes(split.next()));
                }
                for (auto index = el::ItemIndex{0U}; index.isWithin(el::ItemCount{5U}); index.uncheckedIncrement()) {
                    const auto expected =
                        index < el::ItemIndex{3U} ? fields.get(el::ItemIndex::one()) : fields.get(el::ItemIndex{3});
                    REQUIRE_EQUAL(normalizer.normalize(fields.get(index)), expected);
                }
                ++count;
            }
            REQUIRE(count > 19000U);
            REQUIRE_EQUAL(normalizer.normalize("각"_el), "각"_el);
        });
    }

private:
    [[nodiscard]] static auto fromCodes(const el::String &text) -> el::String {
        auto result = el::StringEditor{};
        auto split = el::StringSplitter{text.trimmed(el::CharSet{U' ', U'\t', U'\r'}), U' '};
        const auto reader = tool::unicode::UcdReader{""_el, "NormalizationTest.txt"_el};
        while (!split.isAtEnd()) {
            const auto code = split.next();
            if (!code.isEmpty()) {
                result.append(el::Char{reader.codePoint(code)});
            }
        }
        return result;
    }
};
