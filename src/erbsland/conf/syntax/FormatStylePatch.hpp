// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BooleanForm.hpp"
#include "FormatStyle.hpp"
#include "IndentationStyle.hpp"
#include "MatrixStyle.hpp"
#include "PositionMode.hpp"
#include "ValueWrapping.hpp"
#include "VectorStyle.hpp"
#include "WordCase.hpp"
#include "WordSeparator.hpp"

#include "../../text/Char.hpp"
#include "../../text/StringList.hpp"
#include "../../unit/CpCount.hpp"

#include <optional>

namespace erbsland::conf::syntax {

/// Explicitly specified formatting properties; omitted properties inherit.
/// @tested{SyntaxFormatterTest SyntaxFormatTest SyntaxLiteralFormattingTest}
class FormatStylePatch final {
public:
    // defaults
    FormatStylePatch() = default;

public: // properties
    /// Whole identifier words kept uppercase in title case.
    [[nodiscard]] auto abbreviations() const noexcept -> const std::optional<text::StringList> & {
        return _abbreviations;
    }
    /// Set whole identifier words kept uppercase in title case.
    /// @param value Single regular words; an empty list disables abbreviation handling.
    auto setAbbreviations(text::StringList value) -> FormatStylePatch &;
    /// Case of Boolean literals.
    [[nodiscard]] auto booleanCase() const noexcept -> std::optional<WordCase> { return _booleanCase; }
    /// Set case of Boolean literals.
    auto setBooleanCase(WordCase value) noexcept -> FormatStylePatch &;
    /// Word pair of Boolean literals.
    [[nodiscard]] auto booleanForm() const noexcept -> std::optional<BooleanForm> { return _booleanForm; }
    /// Set word pair of Boolean literals.
    auto setBooleanForm(BooleanForm value) noexcept -> FormatStylePatch &;
    /// Soft maximum line width.
    [[nodiscard]] auto maximumLineLength() const noexcept -> std::optional<unit::CpCount> { return _maximumLineLength; }
    /// Set soft maximum line width.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setMaximumLineLength(unit::CpCount value) noexcept -> FormatStylePatch &;
    /// Case of regular section names.
    [[nodiscard]] auto sectionNameCase() const noexcept -> std::optional<WordCase> { return _sectionNameCase; }
    /// Set case of regular section names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionNameCase(WordCase value) noexcept -> FormatStylePatch &;
    /// Word separator in section names.
    [[nodiscard]] auto sectionNameSeparator() const noexcept -> std::optional<WordSeparator> {
        return _sectionNameSeparator;
    }
    /// Set word separator in section names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionNameSeparator(WordSeparator value) noexcept -> FormatStylePatch &;
    /// Case of regular value names.
    [[nodiscard]] auto valueNameCase() const noexcept -> std::optional<WordCase> { return _valueNameCase; }
    /// Set case of regular value names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setValueNameCase(WordCase value) noexcept -> FormatStylePatch &;
    /// Word separator in value names.
    [[nodiscard]] auto valueNameSeparator() const noexcept -> std::optional<WordSeparator> {
        return _valueNameSeparator;
    }
    /// Set word separator in value names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setValueNameSeparator(WordSeparator value) noexcept -> FormatStylePatch &;
    /// Opt-in case of quoted identifier names.
    [[nodiscard]] auto textNameCase() const noexcept -> std::optional<WordCase> { return _textNameCase; }
    /// Set opt-in case of quoted identifier names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setTextNameCase(WordCase value) noexcept -> FormatStylePatch &;
    /// Word separator in quoted identifier names.
    [[nodiscard]] auto textNameSeparator() const noexcept -> std::optional<WordSeparator> { return _textNameSeparator; }
    /// Set word separator in quoted identifier names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setTextNameSeparator(WordSeparator value) noexcept -> FormatStylePatch &;
    /// Opt-in case of identifiers in text values.
    [[nodiscard]] auto textIdentifierCase() const noexcept -> std::optional<WordCase> { return _textIdentifierCase; }
    /// Set opt-in case of identifiers in text values.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setTextIdentifierCase(WordCase value) noexcept -> FormatStylePatch &;
    /// Word separator in text identifiers.
    [[nodiscard]] auto textIdentifierSeparator() const noexcept -> std::optional<WordSeparator> {
        return _textIdentifierSeparator;
    }
    /// Set word separator in text identifiers.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setTextIdentifierSeparator(WordSeparator value) noexcept -> FormatStylePatch &;
    /// Opening bracket column, or infinite for right alignment.
    [[nodiscard]] auto sectionIndent() const noexcept -> std::optional<unit::CpCount> { return _sectionIndent; }
    /// Set opening bracket column, or infinite for right alignment.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionIndent(unit::CpCount value) noexcept -> FormatStylePatch &;
    /// Trailing decoration width, or infinite for fill.
    [[nodiscard]] auto sectionSuffix() const noexcept -> std::optional<unit::CpCount> { return _sectionSuffix; }
    /// Set trailing decoration width, or infinite for fill.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionSuffix(unit::CpCount value) noexcept -> FormatStylePatch &;
    /// Emit the optional closing star on list sections.
    [[nodiscard]] auto sectionStar() const noexcept -> std::optional<bool> { return _sectionStar; }
    /// Set emit the optional closing star on list sections.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionStar(bool value) noexcept -> FormatStylePatch &;
    /// Space after a section opening bracket.
    [[nodiscard]] auto spaceAfterOpen() const noexcept -> std::optional<bool> { return _spaceAfterOpen; }
    /// Set space after a section opening bracket.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSpaceAfterOpen(bool value) noexcept -> FormatStylePatch &;
    /// Space before a section closing bracket.
    [[nodiscard]] auto spaceBeforeClose() const noexcept -> std::optional<bool> { return _spaceBeforeClose; }
    /// Set space before a section closing bracket.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSpaceBeforeClose(bool value) noexcept -> FormatStylePatch &;
    /// Spaces around section path separators.
    [[nodiscard]] auto spaceAroundDot() const noexcept -> std::optional<bool> { return _spaceAroundDot; }
    /// Set spaces around section path separators.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSpaceAroundDot(bool value) noexcept -> FormatStylePatch &;
    /// Get the explicitly selected non-multiline value placement policy.
    [[nodiscard]] auto valueWrapping() const noexcept -> std::optional<ValueWrapping> { return _valueWrapping; }
    /// Override the placement policy for non-multiline values.
    auto setValueWrapping(ValueWrapping value) noexcept -> FormatStylePatch &;
    /// Vector layout policy.
    [[nodiscard]] auto vectorStyle() const noexcept -> std::optional<VectorStyle> { return _vectorStyle; }
    /// Set vector layout policy.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setVectorStyle(VectorStyle value) noexcept -> FormatStylePatch &;
    /// Matrix column alignment policy.
    [[nodiscard]] auto matrixStyle() const noexcept -> std::optional<MatrixStyle> { return _matrixStyle; }
    /// Set matrix column alignment policy.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setMatrixStyle(MatrixStyle value) noexcept -> FormatStylePatch &;
    /// Spaces after a list bullet.
    [[nodiscard]] auto spacesAfterStar() const noexcept -> std::optional<unit::CpCount> { return _spacesAfterStar; }
    /// Set spaces after a list bullet.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSpacesAfterStar(unit::CpCount value) noexcept -> FormatStylePatch &;
    /// Assignment separator, colon or equals.
    [[nodiscard]] auto separator() const noexcept -> std::optional<text::Char> { return _separator; }
    /// Set assignment separator, colon or equals.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSeparator(text::Char value) noexcept -> FormatStylePatch &;
    /// Placement mode for the assignment separator.
    [[nodiscard]] auto separatorPositionMode() const noexcept -> std::optional<PositionMode> {
        return _separatorPositionMode;
    }
    /// Set placement mode for the assignment separator.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSeparatorPositionMode(PositionMode value) noexcept -> FormatStylePatch &;
    /// Spaces or target column before the separator.
    [[nodiscard]] auto separatorPosition() const noexcept -> std::optional<unit::CpCount> { return _separatorPosition; }
    /// Set spaces or target column before the separator.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSeparatorPosition(unit::CpCount value) noexcept -> FormatStylePatch &;
    /// Placement mode for the first value.
    [[nodiscard]] auto valuePositionMode() const noexcept -> std::optional<PositionMode> { return _valuePositionMode; }
    /// Set placement mode for the first value.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setValuePositionMode(PositionMode value) noexcept -> FormatStylePatch &;
    /// Spaces or target column after the separator.
    [[nodiscard]] auto valuePosition() const noexcept -> std::optional<unit::CpCount> { return _valuePosition; }
    /// Set spaces or target column after the separator.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setValuePosition(unit::CpCount value) noexcept -> FormatStylePatch &;
    /// Continuation indentation character.
    [[nodiscard]] auto indentationStyle() const noexcept -> std::optional<IndentationStyle> {
        return _indentationStyle;
    }
    /// Set continuation indentation character.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setIndentationStyle(IndentationStyle value) noexcept -> FormatStylePatch &;
    /// Positive continuation indentation count.
    [[nodiscard]] auto indentationCount() const noexcept -> std::optional<unit::CpCount> { return _indentationCount; }
    /// Set positive continuation indentation count.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setIndentationCount(unit::CpCount value) noexcept -> FormatStylePatch &;

public: // cascade
    /// Apply specified properties to a complete style.
    /// @param style Style to update.
    void applyTo(FormatStyle &style) const;

private:
    std::optional<text::StringList> _abbreviations;        ///< Title-case abbreviation words.
    std::optional<WordCase> _booleanCase;                  ///< Boolean literal case.
    std::optional<BooleanForm> _booleanForm;               ///< Boolean word pair.
    std::optional<ValueWrapping> _valueWrapping;           ///< Non-multiline value placement override.
    std::optional<unit::CpCount> _maximumLineLength;       ///< Soft maximum line width.
    std::optional<WordCase> _sectionNameCase;              ///< Case of regular section names.
    std::optional<WordSeparator> _sectionNameSeparator;    ///< Word separator in section names.
    std::optional<WordCase> _valueNameCase;                ///< Case of regular value names.
    std::optional<WordSeparator> _valueNameSeparator;      ///< Word separator in value names.
    std::optional<WordCase> _textNameCase;                 ///< Opt-in case of quoted identifier names.
    std::optional<WordSeparator> _textNameSeparator;       ///< Word separator in quoted identifier names.
    std::optional<WordCase> _textIdentifierCase;           ///< Opt-in case of identifiers in text values.
    std::optional<WordSeparator> _textIdentifierSeparator; ///< Word separator in text identifiers.
    std::optional<unit::CpCount> _sectionIndent;           ///< Opening bracket column, or infinite for right alignment.
    std::optional<unit::CpCount> _sectionSuffix;           ///< Trailing decoration width, or infinite for fill.
    std::optional<bool> _sectionStar;                      ///< Emit the optional closing star on list sections.
    std::optional<bool> _spaceAfterOpen;                   ///< Space after a section opening bracket.
    std::optional<bool> _spaceBeforeClose;                 ///< Space before a section closing bracket.
    std::optional<bool> _spaceAroundDot;                   ///< Spaces around section path separators.
    std::optional<VectorStyle> _vectorStyle;               ///< Vector layout policy.
    std::optional<MatrixStyle> _matrixStyle;               ///< Matrix column alignment policy.
    std::optional<unit::CpCount> _spacesAfterStar;         ///< Spaces after a list bullet.
    std::optional<text::Char> _separator;                  ///< Assignment separator, colon or equals.
    std::optional<PositionMode> _separatorPositionMode;    ///< Placement mode for the assignment separator.
    std::optional<unit::CpCount> _separatorPosition;       ///< Spaces or target column before the separator.
    std::optional<PositionMode> _valuePositionMode;        ///< Placement mode for the first value.
    std::optional<unit::CpCount> _valuePosition;           ///< Spaces or target column after the separator.
    std::optional<IndentationStyle> _indentationStyle;     ///< Continuation indentation character.
    std::optional<unit::CpCount> _indentationCount;        ///< Positive continuation indentation count.
};

}
