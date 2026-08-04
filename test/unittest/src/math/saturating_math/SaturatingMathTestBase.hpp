// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/math/SaturatingMath.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <charconv>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

/// Provide CSV parsing helpers for saturating-math tests.
/// @notest{Shared unit-test helper.}
class SaturatingMathTestBase : public el::UnitTest {
public:
    /// Identify a supported integer type.
    enum class TypeId {
        I8,
        U8,
        I16,
        U16,
        I32,
        U32,
        I64,
        U64,
    };

    /// Store one saturating-cast test row.
    struct CastRow {
        TypeId targetType;
        TypeId sourceType;
        std::string value;
        std::string expected;
        bool overflow;
        std::size_t lineNumber{};
    };

    /// Store one saturating-arithmetic test row.
    struct ArithmeticRow {
        TypeId firstType;
        TypeId secondType;
        std::string first;
        std::string second;
        std::string expected;
        bool overflow;
        std::size_t lineNumber{};
    };

protected:
    /// Test whether a CSV line is empty or a comment.
    static auto shouldSkipLine(const std::string &line) -> bool {
        const auto view = trim(line);
        return view.empty() || view.front() == '#';
    }
    /// Split a whitespace-separated CSV row into fields.
    static auto splitFields(const std::string &line) -> std::vector<std::string_view> {
        std::vector<std::string_view> result;
        std::string_view view{line};
        auto index = std::size_t{0};
        while (index < view.size()) {
            while (index < view.size() && isSpace(view[index])) {
                ++index;
            }
            const auto start = index;
            while (index < view.size() && !isSpace(view[index])) {
                ++index;
            }
            if (start < index) {
                result.emplace_back(view.substr(start, index - start));
            }
        }
        return result;
    }
    /// Parse an integer type identifier.
    static auto parseType(std::string_view text) -> TypeId {
        if (text == "i8") {
            return TypeId::I8;
        }
        if (text == "u8") {
            return TypeId::U8;
        }
        if (text == "i16") {
            return TypeId::I16;
        }
        if (text == "u16") {
            return TypeId::U16;
        }
        if (text == "i32") {
            return TypeId::I32;
        }
        if (text == "u32") {
            return TypeId::U32;
        }
        if (text == "i64") {
            return TypeId::I64;
        }
        if (text == "u64") {
            return TypeId::U64;
        }
        throw std::runtime_error{"Unknown integer type."};
    }
    /// Parse a boolean field.
    static auto parseBool(std::string_view text) -> bool {
        if (text == "0") {
            return false;
        }
        if (text == "1") {
            return true;
        }
        throw std::runtime_error{"Invalid boolean value."};
    }
    template <std::integral T>
    /// Parse an integer field of the requested type.
    static auto parseInteger(std::string_view text) -> T {
        if (text.empty()) {
            throw std::runtime_error{"Empty integer value."};
        }
        if constexpr (std::is_unsigned_v<T>) {
            if (text.front() == '-' || text.front() == '+') {
                throw std::runtime_error{"Unexpected sign for unsigned integer."};
            }
            auto value = std::uint64_t{};
            parseChars(text, value);
            if (value > static_cast<std::uint64_t>(std::numeric_limits<T>::max())) {
                throw std::runtime_error{"Unsigned integer value out of range."};
            }
            return static_cast<T>(value);
        } else {
            if (text.front() == '+') {
                throw std::runtime_error{"Unexpected plus sign for integer."};
            }
            auto value = std::int64_t{};
            parseChars(text, value);
            if (value < static_cast<std::int64_t>(std::numeric_limits<T>::min()) ||
                value > static_cast<std::int64_t>(std::numeric_limits<T>::max())) {
                throw std::runtime_error{"Signed integer value out of range."};
            }
            return static_cast<T>(value);
        }
    }
    /// Parse a saturating-cast test row.
    static auto parseCastRow(const std::string &line, std::size_t lineNumber) -> CastRow {
        const auto fields = splitFields(line);
        if (fields.size() != 5) {
            throw std::runtime_error{"Invalid cast row field count."};
        }
        return {
            .targetType = parseType(fields[0]),
            .sourceType = parseType(fields[1]),
            .value = std::string{fields[2]},
            .expected = std::string{fields[3]},
            .overflow = parseBool(fields[4]),
            .lineNumber = lineNumber,
        };
    }
    /// Parse a saturating-arithmetic test row.
    static auto parseArithmeticRow(const std::string &line, std::size_t lineNumber) -> ArithmeticRow {
        const auto fields = splitFields(line);
        if (fields.size() != 6) {
            throw std::runtime_error{"Invalid arithmetic row field count."};
        }
        return {
            .firstType = parseType(fields[0]),
            .secondType = parseType(fields[1]),
            .first = std::string{fields[2]},
            .second = std::string{fields[3]},
            .expected = std::string{fields[4]},
            .overflow = parseBool(fields[5]),
            .lineNumber = lineNumber,
        };
    }
    /// Convert an integer type identifier into its text name.
    static auto typeName(TypeId type) -> std::string_view {
        switch (type) {
        case TypeId::I8:
            return "i8";
        case TypeId::U8:
            return "u8";
        case TypeId::I16:
            return "i16";
        case TypeId::U16:
            return "u16";
        case TypeId::I32:
            return "i32";
        case TypeId::U32:
            return "u32";
        case TypeId::I64:
            return "i64";
        case TypeId::U64:
            return "u64";
        }
        return "?";
    }

private:
    /// Test whether a character is whitespace.
    static auto isSpace(char ch) -> bool { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }
    /// Trim leading and trailing whitespace.
    static auto trim(const std::string &line) -> std::string_view {
        std::string_view view{line};
        while (!view.empty() && isSpace(view.front())) {
            view.remove_prefix(1);
        }
        while (!view.empty() && isSpace(view.back())) {
            view.remove_suffix(1);
        }
        return view;
    }
    template <std::integral T>
    /// Parse an integer field into an existing value.
    static void parseChars(std::string_view text, T &value) {
        const auto *begin = text.data();
        const auto *end = begin + text.size();
        const auto [ptr, error] = std::from_chars(begin, end, value);
        if (error != std::errc{} || ptr != end) {
            throw std::runtime_error{"Invalid integer value."};
        }
    }
};
