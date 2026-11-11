// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatConfig.hpp"

#include <erbsland/all_conf_vr.hpp>
#include <erbsland/all_conf_vr_builder.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/syntax/ValueWrapping.hpp>

namespace tool::elcl_format {

using namespace el::text::literals;
using namespace el::conf::syntax;
using el::conf::vr::RuleType;
using namespace el::conf::vr::builder;

void FormatConfig::addStyleRules(el::conf::vr::RulesBuilderBranch branch) {
    addLiteralStyleRules(branch);
    branch.addRule(
        "names"_el,
        RuleType::Section,
        IsOptional{},
        Title{"Name formatting"_el},
        Description{"Word case and separators for section names and value names."_el});
    branch.addRule(
        cNamesAbbreviations,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        IsOptional{},
        Title{"Title-case abbreviations"_el});
    branch.addRule(
        cSection,
        RuleType::Section,
        IsOptional{},
        Title{"Section formatting"_el},
        Description{"Indentation, suffix and spacing of section declarations."_el});
    branch.addRule(
        "vector"_el,
        RuleType::Section,
        IsOptional{},
        Title{"Vector formatting"_el},
        Description{"Layout and list marker spacing for vector values."_el});
    branch.addRule(
        "matrix"_el,
        RuleType::Section,
        IsOptional{},
        Title{"Matrix formatting"_el},
        Description{"Layout of matrix values."_el});
    branch.addRule(
        "separator"_el,
        RuleType::Section,
        IsOptional{},
        Title{"Value separators"_el},
        Description{"Separator character and alignment of value names and contents."_el});
    branch.addRule(
        "indentation"_el,
        RuleType::Section,
        IsOptional{},
        Title{"Indentation"_el},
        Description{"Whitespace style and indentation width."_el});
    branch.addRule(
        cMaximumLineLength,
        RuleType::Integer,
        IsOptional{},
        Minimum{0},
        Title{"Maximum line length"_el},
        Description{"Maximum character width for wrapping formatted values."_el});
    branch.addRule(
        cNamesSectionCase,
        RuleType::Text,
        IsOptional{},
        Title{"Section name case"_el},
        Description{"Word case applied to section names."_el});
    branch.addRule(
        cNamesSectionSeparator,
        RuleType::Text,
        IsOptional{},
        Title{"Section name separator"_el},
        Description{"Word separator used within section names."_el});
    branch.addRule(
        cNamesValueCase,
        RuleType::Text,
        IsOptional{},
        Title{"Value name case"_el},
        Description{"Word case applied to value names."_el});
    branch.addRule(
        cNamesValueSeparator,
        RuleType::Text,
        IsOptional{},
        Title{"Value name separator"_el},
        Description{"Word separator used within value names."_el});
    branch.addRule(
        cNamesTextNameCase,
        RuleType::Text,
        IsOptional{},
        Title{"Text name case"_el},
        Description{"Word case applied to quoted text names."_el});
    branch.addRule(
        cNamesTextNameSeparator,
        RuleType::Text,
        IsOptional{},
        Title{"Text name separator"_el},
        Description{"Word separator used within quoted text names."_el});
    branch.addRule(
        cNamesTextIdentifierCase,
        RuleType::Text,
        IsOptional{},
        Title{"Text identifier case"_el},
        Description{"Word case applied to text identifiers."_el});
    branch.addRule(
        cNamesTextIdentifierSeparator,
        RuleType::Text,
        IsOptional{},
        Title{"Text identifier separator"_el},
        Description{"Word separator used within text identifiers."_el});
    branch.addRule(
        cSectionIndent,
        RuleType::Alternatives,
        IsOptional{},
        Title{"Section indentation limit"_el},
        Description{"Maximum section depth to indent, or infinite for every depth."_el});
    branch.addAlternative(
        cSectionIndent,
        RuleType::Integer,
        Minimum{0},
        Title{"Section indentation depth"_el},
        Description{"Limit indentation to this section depth."_el});
    branch.addAlternative(
        cSectionIndent,
        RuleType::Text,
        In{"infinite"_el},
        Title{"Unlimited section indentation"_el},
        Description{"Use infinite to indent every section depth."_el});
    branch.addRule(
        cSectionSuffix,
        RuleType::Alternatives,
        IsOptional{},
        Title{"Section suffix limit"_el},
        Description{"Maximum section suffix depth, or infinite for every depth."_el});
    branch.addAlternative(
        cSectionSuffix,
        RuleType::Integer,
        Minimum{0},
        Title{"Section suffix depth"_el},
        Description{"Limit section suffixes to this depth."_el});
    branch.addAlternative(
        cSectionSuffix,
        RuleType::Text,
        In{"infinite"_el},
        Title{"Unlimited section suffix"_el},
        Description{"Use infinite to retain every section suffix depth."_el});
    branch.addRule(
        cSectionStar,
        RuleType::Boolean,
        IsOptional{},
        Title{"List section marker"_el},
        Description{"Whether to include the star marker for list sections."_el});
    branch.addRule(
        cSectionSpaceAfterOpen,
        RuleType::Boolean,
        IsOptional{},
        Title{"Space after opening bracket"_el},
        Description{"Insert a space after the opening section bracket."_el});
    branch.addRule(
        cSectionSpaceBeforeClose,
        RuleType::Boolean,
        IsOptional{},
        Title{"Space before closing bracket"_el},
        Description{"Insert a space before the closing section bracket."_el});
    branch.addRule(
        cSectionSpaceAroundDot,
        RuleType::Boolean,
        IsOptional{},
        Title{"Spaces around section dots"_el},
        Description{"Insert spaces around dots separating section names."_el});
    branch.addRule(
        cValueWrapping,
        RuleType::Text,
        IsOptional{},
        Title{"Value wrapping"_el},
        Description{"Wrapping policy for values that exceed the line width."_el});
    branch.addRule(
        cVectorStyle,
        RuleType::Text,
        IsOptional{},
        Title{"Vector layout"_el},
        Description{"Inline or multiline layout policy for vector values."_el});
    branch.addRule(
        cMatrixStyle,
        RuleType::Text,
        IsOptional{},
        Title{"Matrix layout"_el},
        Description{"Layout policy for matrix rows."_el});
    branch.addRule(
        cVectorSpacesAfterStar,
        RuleType::Integer,
        IsOptional{},
        Minimum{0},
        Title{"Spaces after list marker"_el},
        Description{"Number of spaces between a vector star and its value."_el});
    branch.addRule(
        cSeparatorCharacter,
        RuleType::Text,
        IsOptional{},
        Title{"Separator character"_el},
        Description{"Character separating a value name from its contents."_el});
    branch.addRule(
        cSeparatorPositionMode,
        RuleType::Text,
        IsOptional{},
        Title{"Separator alignment mode"_el},
        Description{"How to determine the separator column."_el});
    branch.addRule(
        cSeparatorPosition,
        RuleType::Integer,
        IsOptional{},
        Minimum{0},
        Title{"Separator column"_el},
        Description{"Target column for aligned value separators."_el});
    branch.addRule(
        cSeparatorValuePositionMode,
        RuleType::Text,
        IsOptional{},
        Title{"Value alignment mode"_el},
        Description{"How to determine the column where value contents begin."_el});
    branch.addRule(
        cSeparatorValuePosition,
        RuleType::Integer,
        IsOptional{},
        Minimum{0},
        Title{"Value column"_el},
        Description{"Target column for aligned value contents."_el});
    branch.addRule(
        cIndentationStyle,
        RuleType::Text,
        IsOptional{},
        Title{"Indentation style"_el},
        Description{"Use spaces or tabs for indentation."_el});
    branch.addRule(
        cIndentationCount,
        RuleType::Integer,
        IsOptional{},
        Minimum{0},
        Title{"Indentation width"_el},
        Description{"Number of indentation units per nesting level."_el});
}

auto FormatConfig::readStyle(const el::conf::ValuePtr &section) -> FormatStylePatch {
    auto patch = FormatStylePatch{};
    if (!section) {
        return patch;
    }
    if (const auto value = section->value(cMaximumLineLength)) {
        patch.setMaximumLineLength(el::CpCount::fromInteger(value->asIntegerOrThrow()));
        if (patch.maximumLineLength()->isZero()) {
            invalid(value);
        }
    }
    readLiteralStyle(section, patch);
    readNameStyle(section, patch);
    readSectionStyle(section, patch);
    readValueStyle(section, patch);
    readSeparatorStyle(section, patch);
    readIndentationStyle(section, patch);
    return patch;
}

auto FormatConfig::readWordCase(const el::conf::ValuePtr &value) -> WordCase {
    const auto text = normalized(value);
    if (text == "keep"_el) {
        return WordCase::Keep;
    }
    if (text == "lower"_el) {
        return WordCase::Lower;
    }
    if (text == "title"_el) {
        return WordCase::Title;
    }
    if (text == "upper"_el) {
        return WordCase::Upper;
    }
    invalid(value);
}

auto FormatConfig::readWordSeparator(const el::conf::ValuePtr &value) -> WordSeparator {
    const auto text = normalized(value);
    if (text == "space"_el) {
        return WordSeparator::Space;
    }
    if (text == "underscore"_el) {
        return WordSeparator::Underscore;
    }
    invalid(value);
}

void FormatConfig::readNameStyle(const el::conf::ValuePtr &section, FormatStylePatch &patch) {
    if (const auto value = section->value(cNamesSectionCase)) {
        patch.setSectionNameCase(readWordCase(value));
    }
    if (const auto value = section->value(cNamesSectionSeparator)) {
        patch.setSectionNameSeparator(readWordSeparator(value));
    }
    if (const auto value = section->value(cNamesValueCase)) {
        patch.setValueNameCase(readWordCase(value));
    }
    if (const auto value = section->value(cNamesValueSeparator)) {
        patch.setValueNameSeparator(readWordSeparator(value));
    }
    if (const auto value = section->value(cNamesTextNameCase)) {
        patch.setTextNameCase(readWordCase(value));
    }
    if (const auto value = section->value(cNamesTextNameSeparator)) {
        patch.setTextNameSeparator(readWordSeparator(value));
    }
    if (const auto value = section->value(cNamesTextIdentifierCase)) {
        patch.setTextIdentifierCase(readWordCase(value));
    }
    if (const auto value = section->value(cNamesTextIdentifierSeparator)) {
        patch.setTextIdentifierSeparator(readWordSeparator(value));
    }
}

void FormatConfig::readSectionStyle(const el::conf::ValuePtr &section, FormatStylePatch &patch) {
    if (const auto value = section->value(cSectionIndent)) {
        if (value->isText()) {
            patch.setSectionIndent(el::CpCount::infinite());
        } else {
            patch.setSectionIndent(el::CpCount::fromInteger(value->asIntegerOrThrow()));
        }
    }
    if (const auto value = section->value(cSectionSuffix)) {
        if (value->isText()) {
            patch.setSectionSuffix(el::CpCount::infinite());
        } else {
            patch.setSectionSuffix(el::CpCount::fromInteger(value->asIntegerOrThrow()));
        }
    }
    if (const auto value = section->value(cSectionStar)) {
        patch.setSectionStar(value->asBooleanOrThrow());
    }
    if (const auto value = section->value(cSectionSpaceAfterOpen)) {
        patch.setSpaceAfterOpen(value->asBooleanOrThrow());
    }
    if (const auto value = section->value(cSectionSpaceBeforeClose)) {
        patch.setSpaceBeforeClose(value->asBooleanOrThrow());
    }
    if (const auto value = section->value(cSectionSpaceAroundDot)) {
        patch.setSpaceAroundDot(value->asBooleanOrThrow());
    }
}

void FormatConfig::readValueStyle(const el::conf::ValuePtr &section, FormatStylePatch &patch) {
    if (const auto value = section->value(cValueWrapping)) {
        const auto text = normalized(value);
        if (text == "keep"_el) {
            patch.setValueWrapping(ValueWrapping::Keep);
        } else if (text == "same_line"_el) {
            patch.setValueWrapping(ValueWrapping::SameLine);
        } else if (text == "next_line"_el) {
            patch.setValueWrapping(ValueWrapping::NextLine);
        } else if (text == "next_line_if_fits"_el) {
            patch.setValueWrapping(ValueWrapping::NextLineIfFits);
        } else if (text == "when_too_long"_el) {
            patch.setValueWrapping(ValueWrapping::WhenTooLong);
        } else {
            invalid(value);
        }
    }
    if (const auto value = section->value(cVectorStyle)) {
        const auto text = normalized(value);
        if (text == "keep"_el) {
            patch.setVectorStyle(VectorStyle::Keep);
        } else if (text == "compact"_el) {
            patch.setVectorStyle(VectorStyle::Compact);
        } else if (text == "multiline"_el) {
            patch.setVectorStyle(VectorStyle::Multiline);
        } else {
            invalid(value);
        }
    }
    if (const auto value = section->value(cMatrixStyle)) {
        const auto text = normalized(value);
        if (text == "keep"_el) {
            patch.setMatrixStyle(MatrixStyle::Keep);
        } else if (text == "align"_el) {
            patch.setMatrixStyle(MatrixStyle::Align);
        } else {
            invalid(value);
        }
    }
    if (const auto value = section->value(cVectorSpacesAfterStar)) {
        patch.setSpacesAfterStar(el::CpCount::fromInteger(value->asIntegerOrThrow()));
        if (patch.spacesAfterStar()->isZero()) {
            invalid(value);
        }
    }
}

void FormatConfig::readSeparatorStyle(const el::conf::ValuePtr &section, FormatStylePatch &patch) {
    if (const auto value = section->value(cSeparatorCharacter)) {
        const auto text = value->asTextOrThrow();
        if (text == ":"_el) {
            patch.setSeparator(el::Char{U':'});
        } else if (text == "="_el) {
            patch.setSeparator(el::Char{U'='});
        } else {
            invalid(value);
        }
    }
    if (const auto value = section->value(cSeparatorPositionMode)) {
        const auto text = normalized(value);
        if (text == "spaces"_el) {
            patch.setSeparatorPositionMode(PositionMode::Spaces);
        } else if (text == "column"_el) {
            patch.setSeparatorPositionMode(PositionMode::Column);
        } else {
            invalid(value);
        }
    }
    if (const auto value = section->value(cSeparatorPosition)) {
        patch.setSeparatorPosition(el::CpCount::fromInteger(value->asIntegerOrThrow()));
    }
    if (const auto value = section->value(cSeparatorValuePositionMode)) {
        const auto text = normalized(value);
        if (text == "spaces"_el) {
            patch.setValuePositionMode(PositionMode::Spaces);
        } else if (text == "column"_el) {
            patch.setValuePositionMode(PositionMode::Column);
        } else {
            invalid(value);
        }
    }
    if (const auto value = section->value(cSeparatorValuePosition)) {
        patch.setValuePosition(el::CpCount::fromInteger(value->asIntegerOrThrow()));
    }
}

void FormatConfig::readIndentationStyle(const el::conf::ValuePtr &section, FormatStylePatch &patch) {
    if (const auto value = section->value(cIndentationStyle)) {
        const auto text = normalized(value);
        if (text == "spaces"_el) {
            patch.setIndentationStyle(IndentationStyle::Spaces);
        } else if (text == "tabs"_el) {
            patch.setIndentationStyle(IndentationStyle::Tabs);
        } else {
            invalid(value);
        }
    }
    if (const auto value = section->value(cIndentationCount)) {
        patch.setIndentationCount(el::CpCount::fromInteger(value->asIntegerOrThrow()));
        if (patch.indentationCount()->isZero()) {
            invalid(value);
        }
    }
}

}
