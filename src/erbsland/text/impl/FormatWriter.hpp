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

#include "../../geometry/Alignment.hpp"
#include "../../geometry/AlignmentFlags.hpp"
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
    /// Format an integer argument with a legacy specification.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto integerFieldText(T value, const LegacyFormatSpec &spec) -> U8StringEditor;
    /// Format an argument using its compiled specification.
    [[nodiscard]] auto fieldText(const FormatArgument &argument, const FormatSpec &spec) -> U8StringEditor;
    /// Format an argument using its default legacy presentation.
    [[nodiscard]] auto defaultFieldText(const FormatArgument &argument, const LegacyFormatSpec &spec) -> U8StringEditor;
    /// Format a text argument using a legacy specification.
    [[nodiscard]] auto textFieldText(const FormatArgument &argument, const LegacyFormatSpec &spec) -> U8StringEditor;
    /// Format an integer argument using a legacy specification.
    [[nodiscard]] auto integerFieldText(const FormatArgument &argument, const LegacyFormatSpec &spec) -> U8StringEditor;
    /// Format a floating-point argument using a legacy specification.
    [[nodiscard]] auto floatFieldText(const FormatArgument &argument, const LegacyFormatSpec &spec) -> U8StringEditor;
    /// Format and escape an argument using a legacy specification.
    [[nodiscard]] auto escapedFieldText(const FormatArgument &argument, const LegacyFormatSpec &spec) -> U8StringEditor;
    /// Format text using a named text specification.
    [[nodiscard]] auto namedTextFieldText(const FormatArgument &argument, const NamedTextFormatSpec &spec)
        -> U8StringEditor;
    /// Format a number using a named number specification.
    [[nodiscard]] auto namedNumberFieldText(const FormatArgument &argument, const NamedNumberFormatSpec &spec)
        -> U8StringEditor;
    /// Format a Boolean using a named Boolean specification.
    [[nodiscard]] auto namedBooleanFieldText(const FormatArgument &argument, const NamedBooleanFormatSpec &spec)
        -> U8StringEditor;
    /// Format bytes using a named byte specification.
    [[nodiscard]] auto namedBytesFieldText(const FormatArgument &argument, const NamedBytesFormatSpec &spec)
        -> U8StringEditor;
    /// Convert one character to writable UTF-8 text.
    [[nodiscard]] auto characterText(Char character) -> U8StringEditor;
    /// Test whether a presentation formats integers.
    [[nodiscard]] static auto isIntegerPresentation(FormatPresentation presentation) noexcept -> bool;
    /// Test whether a presentation formats floating-point values.
    [[nodiscard]] static auto isFloatPresentation(FormatPresentation presentation) noexcept -> bool;
    /// Test whether a presentation formats text.
    [[nodiscard]] static auto isTextPresentation(FormatPresentation presentation) noexcept -> bool;
    /// Convert a legacy specification to an integer format.
    [[nodiscard]] static auto integerFormat(const LegacyFormatSpec &spec) -> IntegerFormat;
    /// Convert a legacy specification to a floating-point format.
    [[nodiscard]] static auto floatFormat(const LegacyFormatSpec &spec) -> FloatFormat;
    /// Derive layout alignment from a legacy specification.
    [[nodiscard]] static auto formattedAlignment(const LegacyFormatSpec &spec, geometry::AlignmentFlag defaultAlignment)
        -> geometry::Alignment;
    /// Derive layout alignment from a named specification.
    [[nodiscard]] static auto formattedAlignment(const NamedLayoutSpec &spec, geometry::AlignmentFlag defaultAlignment)
        -> geometry::Alignment;
    /// Add leading zeroes until text reaches a width.
    [[nodiscard]] static auto zeroPaddedNumericText(const String &text, unit::CpLength width) -> U8StringEditor;
    /// Apply a legacy precision limit to text.
    [[nodiscard]] static auto applyPrecision(const String &text, const LegacyFormatSpec &spec) -> StringEditor;
    /// Apply legacy width, fill, and alignment to text.
    [[nodiscard]] static auto applyLayout(
        U8StringEditor text, const LegacyFormatSpec &spec, geometry::AlignmentFlag defaultAlignment) -> StringEditor;
    /// Apply named width, fill, and alignment to text.
    [[nodiscard]] static auto applyLayout(
        U8StringEditor text,
        const NamedLayoutSpec &spec,
        geometry::AlignmentFlag defaultAlignment,
        bool zeroFill = false) -> U8StringEditor;
    /// Reject legacy format options that are incompatible with text.
    static void requireTextCompatibleSpec(const LegacyFormatSpec &spec);
    /// Reject legacy format options that bytes do not support.
    static void requireDefaultByteSpec(const LegacyFormatSpec &spec);

private:
    AnyStringBuilder &_builder; ///< The destination builder.
};

/// Format an integer value with a legacy specification.
template <math::AnyIntegerType T>
auto FormatWriter::integerFieldText(T value, const LegacyFormatSpec &spec) -> U8StringEditor {
    return U8StringEditor{String::fromInteger(value, integerFormat(spec))};
}

}
