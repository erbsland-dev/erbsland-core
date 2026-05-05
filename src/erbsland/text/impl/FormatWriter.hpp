// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatData.hpp"

#include "../AnyString.hpp"
#include "../Char.hpp"
#include "../FloatFormat.hpp"
#include "../FormatArgument.hpp"
#include "../IntegerFormat.hpp"
#include "../StringBuilder.hpp"
#include "../u8/U8String.hpp"

#include "../../bgeo/Alignment.hpp"
#include "../../bgeo/AlignmentFlags.hpp"
#include "../../math/IntegerTraits.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Writer for applying compiled UTF-8 format parts to a string builder.
/// @tested{U8FormatTest}
class FormatWriter final {
public:
    /// Create a writer for the given string builder.
    explicit FormatWriter(StringBuilder &builder);

public:
    /// Append one compiled format part with the matching argument.
    void appendField(const FormatPart &part, const FormatArgument &argument);

private:
    template <math::AnyIntegerType T>
    [[nodiscard]] auto integerFieldText(T value, const FormatSpec &spec) -> U8String;
    [[nodiscard]] auto defaultFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String;
    [[nodiscard]] auto textFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String;
    [[nodiscard]] auto integerFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String;
    [[nodiscard]] auto floatFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String;
    [[nodiscard]] auto escapedFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8String;
    [[nodiscard]] auto characterText(Char character) -> U8String;
    [[nodiscard]] static auto isIntegerPresentation(FormatPresentation presentation) noexcept -> bool;
    [[nodiscard]] static auto isFloatPresentation(FormatPresentation presentation) noexcept -> bool;
    [[nodiscard]] static auto isTextPresentation(FormatPresentation presentation) noexcept -> bool;
    [[nodiscard]] static auto integerFormat(const FormatSpec &spec) -> IntegerFormat;
    [[nodiscard]] static auto floatFormat(const FormatSpec &spec) -> FloatFormat;
    [[nodiscard]] static auto formattedAlignment(const FormatSpec &spec, bgeo::AlignmentFlag defaultAlignment)
        -> bgeo::Alignment;
    [[nodiscard]] static auto zeroPaddedNumericText(const String &text, unit::CpLength width) -> U8String;
    [[nodiscard]] static auto applyPrecision(const String &text, const FormatSpec &spec) -> String;
    [[nodiscard]] static auto applyLayout(U8String text, const FormatSpec &spec, bgeo::AlignmentFlag defaultAlignment)
        -> String;
    static void requireTextCompatibleSpec(const FormatSpec &spec);

private:
    StringBuilder &_builder;
};

template <math::AnyIntegerType T>
auto FormatWriter::integerFieldText(T value, const FormatSpec &spec) -> U8String {
    auto integerBuilder = StringBuilder{};
    integerBuilder.appendInteger(value, integerFormat(spec));
    return integerBuilder.takeU8String();
}

}
