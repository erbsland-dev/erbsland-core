// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/bgeo/all.hpp>

#include <array>
#include <cstddef>

constexpr auto cSquareSize = el::BlockSize{3, 3};
using SquareArray = std::array<int, cSquareSize.area().toSizeT()>;

struct MagicSquare {
    SquareArray values{};
    int firstValue{};
    int step{};
    int magicSum{};
};

void buildMagicSquare(MagicSquare &square) {
    constexpr auto canonicalSquare = SquareArray{8, 1, 6, 3, 5, 7, 4, 9, 2};

    auto &random = el::application().random();
    const auto symmetry = static_cast<el::Symmetry>(random.selectInteger<int>(0, 7));
    square.firstValue = random.selectInteger<int>(1, 25);
    square.step = random.selectInteger<int>(1, 8);
    cSquareSize.forEach([&](const el::BlockPosition pos) -> void {
        const auto sourceValue = canonicalSquare[cSquareSize.index(cSquareSize.transform(pos, symmetry))];
        square.values[cSquareSize.index(pos)] = square.firstValue + (sourceValue - 1) * square.step;
    });
    square.magicSum = square.values[0] + square.values[1] + square.values[2];
}

void printMagicSquare(const el::TextOutputStreamPtr &outputStream);

/// `StringBuilderStream` is the combination of a `TextOutputStream` and a `StringBuilder`.
/// It allows you to write text output directly into an in-memory string, using the same interface
/// as other text output streams.
/// This is useful for generic stream writing functions: a caller can pass a string builder stream
/// instead of a file or terminal stream to capture the output in memory.
void captureOutput() {
    const auto stringBuilderStream = el::StringBuilderStream::create();
    for (auto i = 0; i < 3; ++i) {
        printMagicSquare(stringBuilderStream);
    }
    el::io::print(stringBuilderStream->takeString());
}

void printMagicSquare(const el::TextOutputStreamPtr &outputStream) {
    MagicSquare square{};
    buildMagicSquare(square);
    const auto intFormat = el::IntegerFormat::decimal().setFieldWidth(el::CpLength{3});
    outputStream->printLine("┌─────┬─────┬─────┐ Sum: "_el, square.magicSum);
    outputStream->printLine(
        intFormat, "│ "_el, square.values[0], " │ "_el, square.values[1], " │ "_el, square.values[2], " │"_el);
    outputStream->printLine("├─────┼─────┼─────┤"_el);
    outputStream->printLine(
        intFormat, "│ "_el, square.values[3], " │ "_el, square.values[4], " │ "_el, square.values[5], " │"_el);
    outputStream->printLine("├─────┼─────┼─────┤"_el);
    outputStream->printLine(
        intFormat, "│ "_el, square.values[6], " │ "_el, square.values[7], " │ "_el, square.values[8], " │"_el);
    outputStream->printLine("└─────┴─────┴─────┘"_el);
}
