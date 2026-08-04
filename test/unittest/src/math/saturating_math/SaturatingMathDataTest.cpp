// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SaturatingMathTestBase.hpp"

#include <erbsland/unittest/FileHelper.hpp>

#include <format>
#include <string>

using namespace el::math;

namespace fh = erbsland::unittest::fh;

TESTED_TARGETS(SaturatingMath)
class SaturatingMathTest final : public UNITTEST_SUBCLASS(SaturatingMathTestBase) {
    enum class Operation {
        Add,
        Subtract,
        Multiply,
        Divide,
        Modulo,
    };

public:
    void testCast() {
        // Test casting operations with saturating math
        WITH_CONTEXT(requireCastFile("data/saturating_math/cast.txt"));
    }
    void testAddition() {
        // Test addition operations with saturating math
        WITH_CONTEXT(requireArithmeticFile("data/saturating_math/add.txt", Operation::Add));
    }
    void testSubtraction() {
        // Test subtraction operations with saturating math
        WITH_CONTEXT(requireArithmeticFile("data/saturating_math/subtract.txt", Operation::Subtract));
    }
    void testMultiplication() {
        // Test multiplication operations with saturating math
        WITH_CONTEXT(requireArithmeticFile("data/saturating_math/multiply.txt", Operation::Multiply));
    }
    void testDivision() {
        // Test division operations with saturating math
        WITH_CONTEXT(requireArithmeticFile("data/saturating_math/divide.txt", Operation::Divide));
    }
    void testModulo() {
        // Test modulo operations with saturating math
        WITH_CONTEXT(requireArithmeticFile("data/saturating_math/modulo.txt", Operation::Modulo));
    }

private:
    void requireCastFile(std::string_view relativePath) {
        const auto lines = fh::readDataLines(relativePath);
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            auto row = parseCastRow(line, lineNumber);
            WITH_CONTEXT(dispatchCast(row));
        }
    }
    void requireArithmeticFile(std::string_view relativePath, Operation operation) {
        const auto lines = fh::readDataLines(relativePath);
        for (auto lineNumber = std::size_t{1}; lineNumber <= lines.size(); ++lineNumber) {
            const auto &line = lines[lineNumber - 1];
            if (shouldSkipLine(line)) {
                continue;
            }
            auto row = parseArithmeticRow(line, lineNumber);
            WITH_CONTEXT(dispatchArithmetic(row, operation));
        }
    }
    void dispatchCast(const CastRow &row) {
        switch (row.targetType) {
        case TypeId::I8:
            WITH_CONTEXT(dispatchCastSource<std::int8_t>(row));
            break;
        case TypeId::U8:
            WITH_CONTEXT(dispatchCastSource<std::uint8_t>(row));
            break;
        case TypeId::I16:
            WITH_CONTEXT(dispatchCastSource<std::int16_t>(row));
            break;
        case TypeId::U16:
            WITH_CONTEXT(dispatchCastSource<std::uint16_t>(row));
            break;
        case TypeId::I32:
            WITH_CONTEXT(dispatchCastSource<std::int32_t>(row));
            break;
        case TypeId::U32:
            WITH_CONTEXT(dispatchCastSource<std::uint32_t>(row));
            break;
        case TypeId::I64:
            WITH_CONTEXT(dispatchCastSource<std::int64_t>(row));
            break;
        case TypeId::U64:
            WITH_CONTEXT(dispatchCastSource<std::uint64_t>(row));
            break;
        }
    }
    template <std::integral Target>
    void dispatchCastSource(const CastRow &row) {
        switch (row.sourceType) {
        case TypeId::I8:
            WITH_CONTEXT(checkCast<Target, std::int8_t>(row));
            break;
        case TypeId::U8:
            WITH_CONTEXT(checkCast<Target, std::uint8_t>(row));
            break;
        case TypeId::I16:
            WITH_CONTEXT(checkCast<Target, std::int16_t>(row));
            break;
        case TypeId::U16:
            WITH_CONTEXT(checkCast<Target, std::uint16_t>(row));
            break;
        case TypeId::I32:
            WITH_CONTEXT(checkCast<Target, std::int32_t>(row));
            break;
        case TypeId::U32:
            WITH_CONTEXT(checkCast<Target, std::uint32_t>(row));
            break;
        case TypeId::I64:
            WITH_CONTEXT(checkCast<Target, std::int64_t>(row));
            break;
        case TypeId::U64:
            WITH_CONTEXT(checkCast<Target, std::uint64_t>(row));
            break;
        }
    }
    template <std::integral Target, std::integral Source>
    void checkCast(const CastRow &row) {
        const auto value = parseInteger<Source>(row.value);
        const auto expected = parseInteger<Target>(row.expected);
        const auto actual = saturatingCast<Target>(value);
        const auto overflow = willCastOverflow<Target>(value);
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void {
                REQUIRE_EQUAL(actual, expected);
                REQUIRE_EQUAL(overflow, row.overflow);
            },
            [&]() -> std::string { return createCastDiagnosticFromRow(row, actual, expected, overflow); });
    }
    void dispatchArithmetic(const ArithmeticRow &row, Operation operation) {
        switch (row.firstType) {
        case TypeId::I8:
            WITH_CONTEXT(dispatchArithmeticSecond<std::int8_t>(row, operation));
            break;
        case TypeId::U8:
            WITH_CONTEXT(dispatchArithmeticSecond<std::uint8_t>(row, operation));
            break;
        case TypeId::I16:
            WITH_CONTEXT(dispatchArithmeticSecond<std::int16_t>(row, operation));
            break;
        case TypeId::U16:
            WITH_CONTEXT(dispatchArithmeticSecond<std::uint16_t>(row, operation));
            break;
        case TypeId::I32:
            WITH_CONTEXT(dispatchArithmeticSecond<std::int32_t>(row, operation));
            break;
        case TypeId::U32:
            WITH_CONTEXT(dispatchArithmeticSecond<std::uint32_t>(row, operation));
            break;
        case TypeId::I64:
            WITH_CONTEXT(dispatchArithmeticSecond<std::int64_t>(row, operation));
            break;
        case TypeId::U64:
            WITH_CONTEXT(dispatchArithmeticSecond<std::uint64_t>(row, operation));
            break;
        }
    }
    template <std::integral First>
    void dispatchArithmeticSecond(const ArithmeticRow &row, Operation operation) {
        switch (row.secondType) {
        case TypeId::I8:
            WITH_CONTEXT(checkArithmetic<First, std::int8_t>(row, operation));
            break;
        case TypeId::U8:
            WITH_CONTEXT(checkArithmetic<First, std::uint8_t>(row, operation));
            break;
        case TypeId::I16:
            WITH_CONTEXT(checkArithmetic<First, std::int16_t>(row, operation));
            break;
        case TypeId::U16:
            WITH_CONTEXT(checkArithmetic<First, std::uint16_t>(row, operation));
            break;
        case TypeId::I32:
            WITH_CONTEXT(checkArithmetic<First, std::int32_t>(row, operation));
            break;
        case TypeId::U32:
            WITH_CONTEXT(checkArithmetic<First, std::uint32_t>(row, operation));
            break;
        case TypeId::I64:
            WITH_CONTEXT(checkArithmetic<First, std::int64_t>(row, operation));
            break;
        case TypeId::U64:
            WITH_CONTEXT(checkArithmetic<First, std::uint64_t>(row, operation));
            break;
        }
    }
    template <std::integral First, std::integral Second>
    void checkArithmetic(const ArithmeticRow &row, Operation operation) {
        const auto first = parseInteger<First>(row.first);
        const auto second = parseInteger<Second>(row.second);
        const auto expected = parseInteger<First>(row.expected);
        const auto actual = arithmeticResult(first, second, operation);
        const auto overflow = arithmeticOverflow(first, second, operation);
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void {
                REQUIRE_EQUAL(actual, expected);
                REQUIRE_EQUAL(overflow, row.overflow);
            },
            [&]() -> std::string {
                return createArithmeticDiagnosticFromRow(row, operationName(operation), actual, expected, overflow);
            });
    }
    template <std::integral First, std::integral Second>
    static auto arithmeticResult(First first, Second second, Operation operation) -> First {
        switch (operation) {
        case Operation::Add:
            return saturatingAdd(first, second);
        case Operation::Subtract:
            return saturatingSubtract(first, second);
        case Operation::Multiply:
            return saturatingMultiply(first, second);
        case Operation::Divide:
            return saturatingDivide(first, second);
        case Operation::Modulo:
            return saturatingModulo(first, second);
        }
        return {};
    }
    template <std::integral First, std::integral Second>
    static auto arithmeticOverflow(First first, Second second, Operation operation) -> bool {
        switch (operation) {
        case Operation::Add:
            return willAddOverflow(first, second);
        case Operation::Subtract:
            return willSubtractOverflow(first, second);
        case Operation::Multiply:
            return willMultiplyOverflow(first, second);
        case Operation::Divide:
            return willDivideOverflow(first, second);
        case Operation::Modulo:
            return willModuloOverflow(first, second);
        }
        return false;
    }
    static auto operationName(Operation operation) -> std::string_view {
        switch (operation) {
        case Operation::Add:
            return "add";
        case Operation::Subtract:
            return "subtract";
        case Operation::Multiply:
            return "multiply";
        case Operation::Divide:
            return "divide";
        case Operation::Modulo:
            return "modulo";
        }
        return "?";
    }
    template <std::integral Value>
    static auto createCastDiagnosticFromRow(const CastRow &row, Value actual, Value expected, bool actualOverflow)
        -> std::string {
        return std::format(
            "cast line: {}\ntypes: {} -> {}\nvalue: {}\nexpected: {}, actual: {}\n"
            "expected overflow: {}, actual overflow: {}\n",
            row.lineNumber,
            typeName(row.sourceType),
            typeName(row.targetType),
            row.value,
            expected,
            actual,
            row.overflow,
            actualOverflow);
    }
    template <std::integral Value>
    static auto createArithmeticDiagnosticFromRow(
        const ArithmeticRow &row, std::string_view operation, Value actual, Value expected, bool actualOverflow) {
        return std::format(
            "{} line: {}\ntypes: {}, {}\noperands: {}, {}\nexpected: {}, actual: {}\n"
            "expected overflow: {}, actual overflow: {}\n",
            operation,
            row.lineNumber,
            typeName(row.firstType),
            typeName(row.secondType),
            row.first,
            row.second,
            expected,
            actual,
            row.overflow,
            actualOverflow);
    }
};
