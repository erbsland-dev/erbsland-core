// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/text/ByteFormat.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringCharReader.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEditor.hpp>
#include <erbsland/time/Date.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/time/Time.hpp>
#include <erbsland/time/TimeWithZone.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>

using namespace el::text::literals;

class ConfTestHelper : public el::UnitTest {
public:
    /// Lines of bytes
    using FileLines = std::vector<std::vector<std::byte>>;

    /// The type of line ending to generate.
    enum class LineBreak : uint8_t { None, LF, CRLF };

public:
    ~ConfTestHelper() override = default;

    /// Convert compact or whitespace-separated hexadecimal test data into a byte block.
    [[nodiscard]] static auto bytesFromHex(const el::text::String &hex) -> el::mem::ByteBlock {
        const auto source = el::text::StringConverter{hex}.toStdString();
        auto editor = el::mem::ByteBlockEditor{};
        auto high = -1;
        const auto digitValue = [](const char character) noexcept -> int {
            if (character >= '0' && character <= '9') {
                return character - '0';
            }
            if (character >= 'a' && character <= 'f') {
                return character - 'a' + 10;
            }
            if (character >= 'A' && character <= 'F') {
                return character - 'A' + 10;
            }
            return -1;
        };
        for (const auto character : source) {
            if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
                continue;
            }
            const auto digit = digitValue(character);
            if (digit < 0) {
                return {};
            }
            if (high < 0) {
                high = digit;
            } else {
                editor.append(el::mem::Byte{static_cast<uint8_t>((high << 4) | digit)});
                high = -1;
            }
        }
        return high < 0 ? el::mem::ByteBlock{editor} : el::mem::ByteBlock{};
    }

    /// Create a Core date from plain test values.
    [[nodiscard]] static auto makeDate(const int year, const int month, const int day) noexcept -> el::time::Date {
        return el::time::Date::fromYearMonthDay(year, month, day);
    }

    /// Create a floating Core time from plain test values.
    [[nodiscard]] static auto makeTime(
        const int hour, const int minute, const int second = 0, const int nanosecond = 0) noexcept -> el::time::Time {
        return el::time::Time{
            el::time::Hour{hour},
            el::time::Minute{minute},
            el::time::Second{second},
            el::time::Nanoseconds{nanosecond}};
    }

    /// Create a Core time with a UTC offset from plain test values.
    [[nodiscard]] static auto makeTimeWithZone(
        const int hour,
        const int minute,
        const int second = 0,
        const int nanosecond = 0,
        const int offsetSeconds = 0) noexcept -> el::time::TimeWithZone {
        return el::time::TimeWithZone{
            el::time::Time{
                el::time::Hour{hour},
                el::time::Minute{minute},
                el::time::Second{second},
                el::time::Nanoseconds{nanosecond}},
            el::time::TimeZone{el::time::Duration{el::time::Seconds{offsetSeconds}}}};
    }

    /// Create a Core date-time from plain local values and a UTC offset in seconds.
    [[nodiscard]] static auto makeDateTime(
        const int year,
        const int month,
        const int day,
        const int hour,
        const int minute,
        const int second = 0,
        const int nanosecond = 0,
        const int offsetSeconds = std::numeric_limits<int>::max()) noexcept -> el::time::DateTime {
        if (offsetSeconds == std::numeric_limits<int>::max()) {
            return el::time::DateTime{
                makeDate(year, month, day), makeTime(hour, minute, second, nanosecond), el::time::TimeZone::local()};
        }
        return el::time::DateTime{
            makeDate(year, month, day), makeTimeWithZone(hour, minute, second, nanosecond, offsetSeconds)};
    }

public:
    /// Test all operators
    ///
    /// Pass six arguments that are:
    /// a1 == b1, a2 == b2, a3 == b3
    /// a1 < a2, a2 < a3, b1 < b2, b2 < b3
    ///
    /// The test always puts a on the left, and b on the right side of the operator.
    template <typename A, typename B>
    void requireAllOperators(const A &a1, const A &a2, const A &a3, const B &b1, const B &b2, const B &b3) {

        // Test ==
        REQUIRE(a1 == b1);
        REQUIRE_FALSE(a1 == b2);
        REQUIRE_FALSE(a1 == b3);
        REQUIRE_FALSE(a2 == b1);
        REQUIRE(a2 == b2);
        REQUIRE_FALSE(a2 == b3);
        REQUIRE_FALSE(a3 == b1);
        REQUIRE_FALSE(a3 == b2);
        REQUIRE(a3 == b3);

        // Test !=
        REQUIRE_FALSE(a1 != b1);
        REQUIRE(a1 != b2);
        REQUIRE(a1 != b3);
        REQUIRE(a2 != b1);
        REQUIRE_FALSE(a2 != b2);
        REQUIRE(a2 != b3);
        REQUIRE(a3 != b1);
        REQUIRE(a3 != b2);
        REQUIRE_FALSE(a3 != b3);

        // Test <
        REQUIRE_FALSE(a1 < b1);
        REQUIRE(a1 < b2);
        REQUIRE(a1 < b3);
        REQUIRE_FALSE(a2 < b1);
        REQUIRE_FALSE(a2 < b2);
        REQUIRE(a2 < b3);
        REQUIRE_FALSE(a3 < b1);
        REQUIRE_FALSE(a3 < b2);
        REQUIRE_FALSE(a3 < b3);

        // Test <=
        REQUIRE(a1 <= b1);
        REQUIRE(a1 <= b2);
        REQUIRE(a1 <= b3);
        REQUIRE_FALSE(a2 <= b1);
        REQUIRE(a2 <= b2);
        REQUIRE(a2 <= b3);
        REQUIRE_FALSE(a3 <= b1);
        REQUIRE_FALSE(a3 <= b2);
        REQUIRE(a3 <= b3);

        // Test >
        REQUIRE_FALSE(a1 > b1);
        REQUIRE_FALSE(a1 > b2);
        REQUIRE_FALSE(a1 > b3);
        REQUIRE(a2 > b1);
        REQUIRE_FALSE(a2 > b2);
        REQUIRE_FALSE(a2 > b3);
        REQUIRE(a3 > b1);
        REQUIRE(a3 > b2);
        REQUIRE_FALSE(a3 > b3);

        // Test >=
        REQUIRE(a1 >= b1);
        REQUIRE_FALSE(a1 >= b2);
        REQUIRE_FALSE(a1 >= b3);
        REQUIRE(a2 >= b1);
        REQUIRE(a2 >= b2);
        REQUIRE_FALSE(a2 >= b3);
        REQUIRE(a3 >= b1);
        REQUIRE(a3 >= b2);
        REQUIRE(a3 >= b3);

        // Test <=>
        REQUIRE((a1 <=> b1) == std::strong_ordering::equal);
        REQUIRE((a1 <=> b2) == std::strong_ordering::less);
        REQUIRE((a1 <=> b3) == std::strong_ordering::less);
        REQUIRE((a2 <=> b1) == std::strong_ordering::greater);
        REQUIRE((a2 <=> b2) == std::strong_ordering::equal);
        REQUIRE((a2 <=> b3) == std::strong_ordering::less);
        REQUIRE((a3 <=> b1) == std::strong_ordering::greater);
        REQUIRE((a3 <=> b2) == std::strong_ordering::greater);
        REQUIRE((a3 <=> b3) == std::strong_ordering::equal);
    }

    template <typename T, std::size_t tSize>
    void requireStrictOrder(const std::array<T, tSize> &valuesInOrder) {
        for (std::size_t i = 0; i < tSize; ++i) {
            for (std::size_t j = 0; j < tSize; ++j) {
                const auto &iValue = valuesInOrder[i];
                const auto &jValue = valuesInOrder[j];
                auto isEqual = i == j;
                auto isLess = i < j;
                auto isLessOrEqual = i <= j;
                auto isGreater = i > j;
                auto isGreaterOrEqual = i >= j;
                REQUIRE(isEqual == (iValue == jValue));
                REQUIRE(isLess == (iValue < jValue));
                REQUIRE(isLessOrEqual == (iValue <= jValue));
                REQUIRE(isGreater == (iValue > jValue));
                REQUIRE(isGreaterOrEqual == (iValue >= jValue));
                REQUIRE((iValue <=> jValue) == (i <=> j));
            }
        }
    }

    /// Generate a random hex string.
    auto generateRandomHex(std::size_t length) -> std::string {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        std::ostringstream oss;
        for (std::size_t i = 0; i < length; ++i) {
            oss << std::setw(2) << std::setfill('0') << std::hex << dis(gen);
        }
        return oss.str();
    }

    /// Generate lines for a test file.
    ///
    /// @param lineLengths An array with lengths, without line break.
    /// @param lineBreak The linebreak in the file.
    /// @param lastLineBreak The linebreak at the end of the file.
    ///
    auto generateLines(
        const std::vector<std::size_t> &lineLengths,
        const LineBreak lineBreak = LineBreak::LF,
        const LineBreak lastLineBreak = LineBreak::None) -> FileLines {

        std::vector<std::vector<std::byte>> lines;
        std::mt19937 gen(928391);
        std::uniform_int_distribution<> dis(0x20, 0x7e);
        for (std::size_t j = 0; j < lineLengths.size(); ++j) {
            const auto lineLength = lineLengths[j];
            std::vector<std::byte> line;
            line.reserve(lineLength);
            for (std::size_t i = 0; i < lineLength; ++i) {
                line.push_back(static_cast<std::byte>(dis(gen)));
            }
            switch ((j == lineLengths.size() - 1) ? lastLineBreak : lineBreak) {
            case LineBreak::CRLF:
                line.push_back(std::byte(0x0d));
                line.push_back(std::byte(0x0a));
                break;
            case LineBreak::LF:
                line.push_back(std::byte(0x0a));
                break;
            default:
                break;
            }
            lines.emplace_back(std::move(line));
        }
        return lines;
    }

    void writeBytesToConsole(std::string label, std::vector<std::byte> bytes) {
        std::stringstream ss;
        ss << label;
        ss << ": ";
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            ss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(bytes[i]);
        }
        consoleWriteLine(ss.str());
    }

public: // helper functions to work with generic test files.
    auto useTestFileDirectory() -> std::filesystem::path {
        if (_temporaryTestFileDirectory == nullptr) {
            auto temporaryTestFileDirectory = std::filesystem::temp_directory_path();
            temporaryTestFileDirectory /= "Erbsland_UnitTest_" + generateRandomHex(4);
            create_directories(temporaryTestFileDirectory);
            // Custom deleter that calls remove_all
            auto deleter = [](std::filesystem::path *path) {
                if (path) {
                    try {
                        std::filesystem::remove_all(*path);
                    } catch (std::exception &) { // NOLINT(*-empty-catch)
                        // ignore any exception.
                    }
                    delete path; // Explicitly delete the path pointer
                }
            };
            _temporaryTestFileDirectory =
                std::shared_ptr<std::filesystem::path>(new std::filesystem::path{temporaryTestFileDirectory}, deleter);
        }
        return *_temporaryTestFileDirectory;
    }

    auto cleanUpTestFileDirectory() { _temporaryTestFileDirectory.reset(); }

    auto createTemporaryFilePath() -> std::filesystem::path {
        auto result = useTestFileDirectory();
        result /= generateRandomHex(8) + ".txt";
        return result;
    }

    void setTestContents(const erbsland::text::String &text) {
        testContents = el::text::StringEditor{
            std::format("UTF-8 Text, {} bytes (·=space ↦=tab ↲=newline ●=EOF):\n", text.length().toSizeT())};
        auto reader = el::text::StringCharReader{text};
        for (auto character = reader.read(); character != el::text::Char::endOfData(); character = reader.read()) {
            if (character == el::text::Char{U' '}) {
                testContents.append(el::text::String{"·"_el});
            } else if (character == el::text::Char{U'\t'}) {
                testContents.append(el::text::String{"↦"_el});
            } else if (character == el::text::Char{U'\n'}) {
                testContents.append(el::text::String{"↲\n"_el});
            } else {
                testContents.append(character);
            }
        }
        testContents.append(el::text::String{"●"_el});
    }

    void setTestContents(const erbsland::mem::ByteBlock &content) {
        testContents = el::text::StringEditor{std::format("Binary data, {} bytes:\n", content.length().toSizeT())};
        testContents.append(el::text::String::fromByteBlock(content, el::text::ByteFormat::separated()));
    }

    void setTestContents(const FileLines &content) {
        testContents = el::text::StringEditor{std::format("Artificial line data, {} lines:\n", content.size())};
        int counter = 0;
        for (const auto &line : content) {
            testContents.append(el::text::String{std::format("Line {}: {} bytes\n", counter++, line.size())});
        }
    }

    auto createTestFile(const erbsland::text::String &text) -> std::filesystem::path {
        auto filePath = createTemporaryFilePath();
        std::ofstream stream(filePath, std::ios::binary);
        stream << el::text::StringConverter{text}.toStdString();
        stream.close();
        setTestContents(text);
        return filePath;
    }

    auto createTestFile(const erbsland::mem::ByteBlock &content) -> std::filesystem::path {
        auto filePath = createTemporaryFilePath();
        std::ofstream stream(filePath, std::ios::binary);
        const auto bytes = content.toCharVector();
        stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        stream.close();
        setTestContents(content);
        return filePath;
    }

    auto createTestFile(const FileLines &content) -> std::filesystem::path {
        auto filePath = createTemporaryFilePath();
        std::ofstream stream(filePath, std::ios::binary);
        for (const auto &line : content) {
            stream.write(reinterpret_cast<const char *>(line.data()), static_cast<std::streamsize>(line.size()));
        }
        stream.close();
        testContents = el::text::StringEditor{std::format("Artificial line data, {} lines:\n", content.size())};
        int counter = 0;
        for (const auto &line : content) {
            testContents.append(el::text::String{std::format("Line {}: {} bytes\n", counter++, line.size())});
            counter += 1;
        }
        return filePath;
    }

    auto createTestMemorySource(const erbsland::text::String &text) -> erbsland::conf::SourcePtr {
        setTestContents(text);
        return erbsland::conf::Source::fromString(text);
    }

protected:
    erbsland::text::StringEditor testContents;

private:
    std::shared_ptr<std::filesystem::path> _temporaryTestFileDirectory{};
};

/// Convert hexadecimal test data in tests that do not derive from `ConfTestHelper`.
[[nodiscard]] inline auto bytesFromHex(const el::text::String &hex) -> el::mem::ByteBlock {
    return ConfTestHelper::bytesFromHex(hex);
}

/// Create a Core date in tests that do not derive from `ConfTestHelper`.
[[nodiscard]] inline auto makeDate(const int year, const int month, const int day) noexcept -> el::time::Date {
    return ConfTestHelper::makeDate(year, month, day);
}

/// Create a floating Core time in tests that do not derive from `ConfTestHelper`.
[[nodiscard]] inline auto makeTime(
    const int hour, const int minute, const int second = 0, const int nanosecond = 0) noexcept -> el::time::Time {
    return ConfTestHelper::makeTime(hour, minute, second, nanosecond);
}

[[nodiscard]] inline auto makeTimeWithZone(
    const int hour,
    const int minute,
    const int second = 0,
    const int nanosecond = 0,
    const int offsetSeconds = 0) noexcept -> el::time::TimeWithZone {
    return ConfTestHelper::makeTimeWithZone(hour, minute, second, nanosecond, offsetSeconds);
}

/// Create a Core date-time in tests that do not derive from `ConfTestHelper`.
[[nodiscard]] inline auto makeDateTime(
    const int year,
    const int month,
    const int day,
    const int hour,
    const int minute,
    const int second = 0,
    const int nanosecond = 0,
    const int offsetSeconds = std::numeric_limits<int>::max()) noexcept -> el::time::DateTime {
    return ConfTestHelper::makeDateTime(year, month, day, hour, minute, second, nanosecond, offsetSeconds);
}
