// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatData.hpp"

#include "../AnyStringBuilder.hpp"
#include "../AnyStringEditor.hpp"
#include "../Char.hpp"
#include "../FloatFormat.hpp"
#include "../FormatArgument.hpp"
#include "../IntegerFormat.hpp"
#include "../u8/U8StringEditor.hpp"

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
    explicit FormatWriter(AnyStringBuilder &builder);

public:
    /// Append one compiled format part with the matching argument.
    void appendField(const FormatPart &part, const FormatArgument &argument);

private:
    template <math::AnyIntegerType T>
    [[nodiscard]] auto integerFieldText(T value, const FormatSpec &spec) -> U8StringEditor;
    [[nodiscard]] auto defaultFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor;
    [[nodiscard]] auto textFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor;
    [[nodiscard]] auto integerFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor;
    [[nodiscard]] auto floatFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor;
    [[nodiscard]] auto escapedFieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor;
    [[nodiscard]] auto characterText(Char character) -> U8StringEditor;
    [[nodiscard]] static auto isIntegerPresentation(FormatPresentation presentation) noexcept -> bool;
    [[nodiscard]] static auto isFloatPresentation(FormatPresentation presentation) noexcept -> bool;
    [[nodiscard]] static auto isTextPresentation(FormatPresentation presentation) noexcept -> bool;
    [[nodiscard]] static auto integerFormat(const FormatSpec &spec) -> IntegerFormat;
    [[nodiscard]] static auto floatFormat(const FormatSpec &spec) -> FloatFormat;
    [[nodiscard]] static auto formattedAlignment(const FormatSpec &spec, bgeo::AlignmentFlag defaultAlignment)
        -> bgeo::Alignment;
    [[nodiscard]] static auto zeroPaddedNumericText(const String &text, unit::CpLength width) -> U8StringEditor;
    [[nodiscard]] static auto applyPrecision(const String &text, const FormatSpec &spec) -> StringEditor;
    [[nodiscard]] static auto applyLayout(
        U8StringEditor text, const FormatSpec &spec, bgeo::AlignmentFlag defaultAlignment) -> StringEditor;
    static void requireTextCompatibleSpec(const FormatSpec &spec);

private:
    AnyStringBuilder &_builder;
};

template <math::AnyIntegerType T>
auto FormatWriter::integerFieldText(T value, const FormatSpec &spec) -> U8StringEditor {
    return U8StringEditor{String::fromInteger(value, integerFormat(spec))};
}

}
