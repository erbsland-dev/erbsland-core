// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/NormalizationForm.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/u32/U32String.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <charconv>
#include <format>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace el::text;

TESTED_TARGETS(NormalizationForm U32String)
class UnicodeNormalizationConformanceTest final : public el::UnitTest {
private:
    std::size_t _lineNumber{0};

    [[nodiscard]] static auto parseCodePoints(const std::string_view field) -> U32String {
        auto result = U32StringEditor{};
        auto position = std::size_t{0};
        while (position < field.size()) {
            while (position < field.size() && field[position] == ' ') {
                ++position;
            }
            if (position == field.size()) {
                break;
            }
            auto end = field.find(' ', position);
            if (end == std::string_view::npos) {
                end = field.size();
            }
            auto value = uint32_t{0};
            const auto parsed = std::from_chars(field.data() + position, field.data() + end, value, 16);
            if (parsed.ec != std::errc{} || parsed.ptr != field.data() + end) {
                throw std::runtime_error{"Invalid code point in NormalizationTest.txt"};
            }
            result.append(Char{static_cast<char32_t>(value)});
            position = end;
        }
        return result;
    }

    [[nodiscard]] static auto parseColumns(const std::string_view line) -> std::vector<U32String> {
        auto result = std::vector<U32String>{};
        auto position = std::size_t{0};
        while (result.size() < 5U) {
            const auto end = line.find(';', position);
            if (end == std::string_view::npos) {
                break;
            }
            result.push_back(parseCodePoints(line.substr(position, end - position)));
            position = end + 1U;
        }
        return result;
    }

    void requireNormalized(const U32String &source, const NormalizationForm form, const U32String &expected) {
        REQUIRE_EQUAL(source.normalized(form), expected);
    }

    void requireConformance(const std::span<const U32String, 5> columns) {
        const auto &source = columns[0];
        const auto &nfc = columns[1];
        const auto &nfd = columns[2];
        const auto &nfkc = columns[3];
        const auto &nfkd = columns[4];

        WITH_CONTEXT(requireNormalized(source, NormalizationForm::Nfc, nfc));
        WITH_CONTEXT(requireNormalized(nfc, NormalizationForm::Nfc, nfc));
        WITH_CONTEXT(requireNormalized(nfd, NormalizationForm::Nfc, nfc));
        WITH_CONTEXT(requireNormalized(nfkc, NormalizationForm::Nfc, nfkc));
        WITH_CONTEXT(requireNormalized(nfkd, NormalizationForm::Nfc, nfkc));

        WITH_CONTEXT(requireNormalized(source, NormalizationForm::Nfd, nfd));
        WITH_CONTEXT(requireNormalized(nfc, NormalizationForm::Nfd, nfd));
        WITH_CONTEXT(requireNormalized(nfd, NormalizationForm::Nfd, nfd));
        WITH_CONTEXT(requireNormalized(nfkc, NormalizationForm::Nfd, nfkd));
        WITH_CONTEXT(requireNormalized(nfkd, NormalizationForm::Nfd, nfkd));

        WITH_CONTEXT(requireNormalized(source, NormalizationForm::Nfkc, nfkc));
        WITH_CONTEXT(requireNormalized(nfc, NormalizationForm::Nfkc, nfkc));
        WITH_CONTEXT(requireNormalized(nfd, NormalizationForm::Nfkc, nfkc));
        WITH_CONTEXT(requireNormalized(nfkc, NormalizationForm::Nfkc, nfkc));
        WITH_CONTEXT(requireNormalized(nfkd, NormalizationForm::Nfkc, nfkc));

        WITH_CONTEXT(requireNormalized(source, NormalizationForm::Nfkd, nfkd));
        WITH_CONTEXT(requireNormalized(nfc, NormalizationForm::Nfkd, nfkd));
        WITH_CONTEXT(requireNormalized(nfd, NormalizationForm::Nfkd, nfkd));
        WITH_CONTEXT(requireNormalized(nfkc, NormalizationForm::Nfkd, nfkd));
        WITH_CONTEXT(requireNormalized(nfkd, NormalizationForm::Nfkd, nfkd));
    }

public:
    auto additionalErrorMessages() -> std::string override {
        return std::format("NormalizationTest.txt line: {}", _lineNumber);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testUnicode17Conformance() {
        const auto lines = el::unittest::fh::readDataLines("data/unicode/NormalizationTest.txt", 4'000'000U);
        for (const auto &line : lines) {
            ++_lineNumber;
            const auto content = std::string_view{line}.substr(0, line.find('#'));
            if (content.empty() || content.front() == '@') {
                continue;
            }
            const auto columns = parseColumns(content);
            REQUIRE_EQUAL(columns.size(), 5U);
            runWithContext(SOURCE_LOCATION(), [&]() -> void {
                requireConformance(std::span<const U32String, 5>{columns.data(), columns.size()});
            });
        }
    }
};
