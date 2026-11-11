// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/conf/syntax/Format.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>

namespace tool::elcl_format {

using namespace el::text::literals;

/// Validate and translate an independent formatter configuration.
class FormatConfig final {
    /// Style key listing title-case abbreviations.
    static constexpr auto cNamesAbbreviations = "names.abbreviations"_el;
    /// Style key selecting Boolean case.
    static constexpr auto cBooleanCase = "boolean.case"_el;
    /// Style key selecting Boolean word pairs.
    static constexpr auto cBooleanForm = "boolean.form"_el;
    /// Configuration path containing the base formatting style applied before conditional rules.
    static constexpr auto cElclFormatBase = "elcl_format.base"_el;
    /// Configuration path containing the ordered conditional formatting rules.
    static constexpr auto cElclFormatRules = "elcl_format.rules"_el;
    /// Validation-rule path describing each conditional formatting entry.
    static constexpr auto cElclFormatRulesVrEntry = "elcl_format.rules.vr_entry"_el;
    /// Style key setting the number of spaces or tab stops per indentation level.
    static constexpr auto cIndentationCount = "indentation.count"_el;
    /// Style key selecting spaces or tabs for indentation.
    static constexpr auto cIndentationStyle = "indentation.style"_el;
    /// Selector key restricting a rule to sections or assignments.
    static constexpr auto cKind = "kind"_el;
    /// Selector key restricting a rule by list-section status.
    static constexpr auto cListSection = "list_section"_el;
    /// Style key selecting whether matrix columns are aligned.
    static constexpr auto cMatrixStyle = "matrix.style"_el;
    /// Style key setting the preferred maximum line length for wrapping.
    static constexpr auto cMaximumLineLength = "maximum_line_length"_el;
    /// Selector key restricting a rule by meta-value status.
    static constexpr auto cMetaValue = "meta_value"_el;
    /// Style key controlling word case in section names.
    static constexpr auto cNamesSectionCase = "names.section_case"_el;
    /// Style key selecting the word separator in section names.
    static constexpr auto cNamesSectionSeparator = "names.section_separator"_el;
    /// Style key controlling word case in identifiers within text values.
    static constexpr auto cNamesTextIdentifierCase = "names.text_identifier_case"_el;
    /// Style key selecting the word separator in identifiers within text values.
    static constexpr auto cNamesTextIdentifierSeparator = "names.text_identifier_separator"_el;
    /// Style key controlling word case in quoted text names.
    static constexpr auto cNamesTextNameCase = "names.text_name_case"_el;
    /// Style key selecting the word separator in quoted text names.
    static constexpr auto cNamesTextNameSeparator = "names.text_name_separator"_el;
    /// Style key controlling word case in value names.
    static constexpr auto cNamesValueCase = "names.value_case"_el;
    /// Style key selecting the word separator in value names.
    static constexpr auto cNamesValueSeparator = "names.value_separator"_el;
    /// Selector key listing configuration-path patterns to match.
    static constexpr auto cPaths = "paths"_el;
    /// Selector key restricting a rule by relative-section status.
    static constexpr auto cRelativeSection = "relative_section"_el;
    /// Selector value identifying section statements rather than assignments.
    static constexpr auto cSection = "section"_el;
    /// Style key setting the opening section-bracket column or right alignment.
    static constexpr auto cSectionIndent = "section.indent"_el;
    /// Style key controlling spacing after the opening section bracket.
    static constexpr auto cSectionSpaceAfterOpen = "section.space_after_open"_el;
    /// Style key controlling spacing around dots in section paths.
    static constexpr auto cSectionSpaceAroundDot = "section.space_around_dot"_el;
    /// Style key controlling spacing before the closing section bracket.
    static constexpr auto cSectionSpaceBeforeClose = "section.space_before_close"_el;
    /// Style key controlling the star in list-section headers.
    static constexpr auto cSectionStar = "section.star"_el;
    /// Style key setting the trailing section-decoration width or filling the remaining line.
    static constexpr auto cSectionSuffix = "section.suffix"_el;
    /// Selector key setting a strict lower bound on the number of section-name components.
    static constexpr auto cSectionNamesMoreThan = "section_names_more_than"_el;
    /// Rule-entry key containing the conditions that select statements.
    static constexpr auto cSelector = "selector"_el;
    /// Style key choosing the assignment separator, colon or equals sign.
    static constexpr auto cSeparatorCharacter = "separator.character"_el;
    /// Style key setting the assignment separator spacing or column.
    static constexpr auto cSeparatorPosition = "separator.position"_el;
    /// Style key interpreting separator positions as spacing or absolute columns.
    static constexpr auto cSeparatorPositionMode = "separator.position_mode"_el;
    /// Style key setting the value spacing or column after the separator.
    static constexpr auto cSeparatorValuePosition = "separator.value_position"_el;
    /// Style key interpreting value positions as spacing or absolute columns.
    static constexpr auto cSeparatorValuePositionMode = "separator.value_position_mode"_el;
    /// Rule-entry key containing the formatting properties to override.
    static constexpr auto cStyle = "style"_el;
    /// Selector key listing the assignment value types to match.
    static constexpr auto cValueTypes = "value_types"_el;
    /// Style key setting the spaces after a multiline vector entry marker.
    static constexpr auto cVectorSpacesAfterStar = "vector.spaces_after_star"_el;
    /// Style key selecting when assignment values move to the next line.
    static constexpr auto cValueWrapping = "value_wrapping"_el;
    /// Style key selecting compact or multiline vector layout.
    static constexpr auto cVectorStyle = "vector.style"_el;

public:
    /// Select a formatter configuration file.
    /// @param path Configuration path.
    explicit FormatConfig(el::Path path) : _path{std::move(path)} {}
    /// Read and validate the selected file.
    /// @return Validated format.
    auto read() const -> el::conf::syntax::Format;

private:
    /// Declare all optional formatting properties.
    static void addStyleRules(el::conf::vr::RulesBuilderBranch branch);
    /// Declare abbreviation and Boolean settings.
    static void addLiteralStyleRules(el::conf::vr::RulesBuilderBranch &branch);
    /// Read abbreviation and Boolean settings.
    static void readLiteralStyle(const el::conf::ValuePtr &section, el::conf::syntax::FormatStylePatch &patch);
    /// Translate explicit style settings into a property patch.
    static auto readStyle(const el::conf::ValuePtr &section) -> el::conf::syntax::FormatStylePatch;
    /// Read the name formatting properties.
    static void readNameStyle(const el::conf::ValuePtr &section, el::conf::syntax::FormatStylePatch &patch);
    /// Read the section formatting properties.
    static void readSectionStyle(const el::conf::ValuePtr &section, el::conf::syntax::FormatStylePatch &patch);
    /// Read the value formatting properties.
    static void readValueStyle(const el::conf::ValuePtr &section, el::conf::syntax::FormatStylePatch &patch);
    /// Read the separator formatting properties.
    static void readSeparatorStyle(const el::conf::ValuePtr &section, el::conf::syntax::FormatStylePatch &patch);
    /// Read the indentation formatting properties.
    static void readIndentationStyle(const el::conf::ValuePtr &section, el::conf::syntax::FormatStylePatch &patch);
    /// Parse a name casing value with the existing diagnostic context.
    static auto readWordCase(const el::conf::ValuePtr &value) -> el::conf::syntax::WordCase;
    /// Parse a name word separator with the existing diagnostic context.
    static auto readWordSeparator(const el::conf::ValuePtr &value) -> el::conf::syntax::WordSeparator;
    /// Parse one supported selector value type, retaining the configuration error location.
    static auto readValueType(const el::String &name, const el::conf::ValuePtr &value) -> el::conf::ValueType;
    /// Compile the conjunctive rule selector.
    static auto readSelector(const el::conf::ValuePtr &section) -> el::conf::syntax::Selector;
    /// Read a case-insensitive identifier setting.
    static auto normalized(const el::conf::ValuePtr &value) -> el::String;
    /// Report an unsupported configuration value.
    [[noreturn]] static void invalid(const el::conf::ValuePtr &value);

private:
    el::Path _path; ///< Selected configuration file.
};

}
