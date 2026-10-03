// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IndentationStyle.hpp"
#include "MatrixStyle.hpp"
#include "PositionMode.hpp"
#include "ValueWrapping.hpp"
#include "VectorStyle.hpp"
#include "WordCase.hpp"
#include "WordSeparator.hpp"

#include "../../text/Char.hpp"
#include "../../unit/CpCount.hpp"

namespace erbsland::conf::syntax {

/// Complete formatting properties with repository-style defaults.
/// @tested{SyntaxFormatterTest SyntaxFormatTest}
class FormatStyle final {
public:
    // defaults
    FormatStyle() = default;

public: // properties
    /// Soft maximum line width.
    [[nodiscard]] auto maximumLineLength() const noexcept -> unit::CpCount { return _maximumLineLength; }
    /// Set soft maximum line width.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setMaximumLineLength(unit::CpCount value) noexcept -> FormatStyle &;
    /// Case of regular section names.
    [[nodiscard]] auto sectionNameCase() const noexcept -> WordCase { return _sectionNameCase; }
    /// Set case of regular section names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionNameCase(WordCase value) noexcept -> FormatStyle &;
    /// Word separator in section names.
    [[nodiscard]] auto sectionNameSeparator() const noexcept -> WordSeparator { return _sectionNameSeparator; }
    /// Set word separator in section names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionNameSeparator(WordSeparator value) noexcept -> FormatStyle &;
    /// Case of regular value names.
    [[nodiscard]] auto valueNameCase() const noexcept -> WordCase { return _valueNameCase; }
    /// Set case of regular value names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setValueNameCase(WordCase value) noexcept -> FormatStyle &;
    /// Word separator in value names.
    [[nodiscard]] auto valueNameSeparator() const noexcept -> WordSeparator { return _valueNameSeparator; }
    /// Set word separator in value names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setValueNameSeparator(WordSeparator value) noexcept -> FormatStyle &;
    /// Opt-in case of quoted identifier names.
    [[nodiscard]] auto textNameCase() const noexcept -> WordCase { return _textNameCase; }
    /// Set opt-in case of quoted identifier names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setTextNameCase(WordCase value) noexcept -> FormatStyle &;
    /// Word separator in quoted identifier names.
    [[nodiscard]] auto textNameSeparator() const noexcept -> WordSeparator { return _textNameSeparator; }
    /// Set word separator in quoted identifier names.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setTextNameSeparator(WordSeparator value) noexcept -> FormatStyle &;
    /// Opt-in case of identifiers in text values.
    [[nodiscard]] auto textIdentifierCase() const noexcept -> WordCase { return _textIdentifierCase; }
    /// Set opt-in case of identifiers in text values.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setTextIdentifierCase(WordCase value) noexcept -> FormatStyle &;
    /// Word separator in text identifiers.
    [[nodiscard]] auto textIdentifierSeparator() const noexcept -> WordSeparator { return _textIdentifierSeparator; }
    /// Set word separator in text identifiers.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setTextIdentifierSeparator(WordSeparator value) noexcept -> FormatStyle &;
    /// Opening bracket column, or infinite for right alignment.
    [[nodiscard]] auto sectionIndent() const noexcept -> unit::CpCount { return _sectionIndent; }
    /// Set opening bracket column, or infinite for right alignment.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionIndent(unit::CpCount value) noexcept -> FormatStyle &;
    /// Trailing decoration width, or infinite for fill.
    [[nodiscard]] auto sectionSuffix() const noexcept -> unit::CpCount { return _sectionSuffix; }
    /// Set trailing decoration width, or infinite for fill.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionSuffix(unit::CpCount value) noexcept -> FormatStyle &;
    /// Emit the optional closing star on list sections.
    [[nodiscard]] auto sectionStar() const noexcept -> bool { return _sectionStar; }
    /// Set emit the optional closing star on list sections.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSectionStar(bool value) noexcept -> FormatStyle &;
    /// Space after a section opening bracket.
    [[nodiscard]] auto spaceAfterOpen() const noexcept -> bool { return _spaceAfterOpen; }
    /// Set space after a section opening bracket.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSpaceAfterOpen(bool value) noexcept -> FormatStyle &;
    /// Space before a section closing bracket.
    [[nodiscard]] auto spaceBeforeClose() const noexcept -> bool { return _spaceBeforeClose; }
    /// Set space before a section closing bracket.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSpaceBeforeClose(bool value) noexcept -> FormatStyle &;
    /// Spaces around section path separators.
    [[nodiscard]] auto spaceAroundDot() const noexcept -> bool { return _spaceAroundDot; }
    /// Set spaces around section path separators.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSpaceAroundDot(bool value) noexcept -> FormatStyle &;
    /// Get the placement policy for non-multiline values.
    [[nodiscard]] auto valueWrapping() const noexcept -> ValueWrapping { return _valueWrapping; }
    /// Select the placement policy for non-multiline values.
    auto setValueWrapping(ValueWrapping value) noexcept -> FormatStyle &;
    /// Vector layout policy.
    [[nodiscard]] auto vectorStyle() const noexcept -> VectorStyle { return _vectorStyle; }
    /// Set vector layout policy.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setVectorStyle(VectorStyle value) noexcept -> FormatStyle &;
    /// Matrix column alignment policy.
    [[nodiscard]] auto matrixStyle() const noexcept -> MatrixStyle { return _matrixStyle; }
    /// Set matrix column alignment policy.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setMatrixStyle(MatrixStyle value) noexcept -> FormatStyle &;
    /// Spaces after a list bullet.
    [[nodiscard]] auto spacesAfterStar() const noexcept -> unit::CpCount { return _spacesAfterStar; }
    /// Set spaces after a list bullet.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSpacesAfterStar(unit::CpCount value) noexcept -> FormatStyle &;
    /// Assignment separator, colon or equals.
    [[nodiscard]] auto separator() const noexcept -> text::Char { return _separator; }
    /// Set assignment separator, colon or equals.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSeparator(text::Char value) noexcept -> FormatStyle &;
    /// Placement mode for the assignment separator.
    [[nodiscard]] auto separatorPositionMode() const noexcept -> PositionMode { return _separatorPositionMode; }
    /// Set placement mode for the assignment separator.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSeparatorPositionMode(PositionMode value) noexcept -> FormatStyle &;
    /// Spaces or target column before the separator.
    [[nodiscard]] auto separatorPosition() const noexcept -> unit::CpCount { return _separatorPosition; }
    /// Set spaces or target column before the separator.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setSeparatorPosition(unit::CpCount value) noexcept -> FormatStyle &;
    /// Placement mode for the first value.
    [[nodiscard]] auto valuePositionMode() const noexcept -> PositionMode { return _valuePositionMode; }
    /// Set placement mode for the first value.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setValuePositionMode(PositionMode value) noexcept -> FormatStyle &;
    /// Spaces or target column after the separator.
    [[nodiscard]] auto valuePosition() const noexcept -> unit::CpCount { return _valuePosition; }
    /// Set spaces or target column after the separator.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setValuePosition(unit::CpCount value) noexcept -> FormatStyle &;
    /// Continuation indentation character.
    [[nodiscard]] auto indentationStyle() const noexcept -> IndentationStyle { return _indentationStyle; }
    /// Set continuation indentation character.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setIndentationStyle(IndentationStyle value) noexcept -> FormatStyle &;
    /// Positive continuation indentation count.
    [[nodiscard]] auto indentationCount() const noexcept -> unit::CpCount { return _indentationCount; }
    /// Set positive continuation indentation count.
    /// @param value New property value.
    /// @return This object for chaining.
    auto setIndentationCount(unit::CpCount value) noexcept -> FormatStyle &;

private:
    ValueWrapping _valueWrapping{ValueWrapping::NextLineIfFits};  ///< Non-multiline value placement.
    unit::CpCount _maximumLineLength{unit::CpCount{118}};         ///< Soft maximum line width.
    WordCase _sectionNameCase{WordCase::Title};                   ///< Case of regular section names.
    WordSeparator _sectionNameSeparator{WordSeparator::Space};    ///< Word separator in section names.
    WordCase _valueNameCase{WordCase::Title};                     ///< Case of regular value names.
    WordSeparator _valueNameSeparator{WordSeparator::Space};      ///< Word separator in value names.
    WordCase _textNameCase{WordCase::Keep};                       ///< Opt-in case of quoted identifier names.
    WordSeparator _textNameSeparator{WordSeparator::Space};       ///< Word separator in quoted identifier names.
    WordCase _textIdentifierCase{WordCase::Keep};                 ///< Opt-in case of identifiers in text values.
    WordSeparator _textIdentifierSeparator{WordSeparator::Space}; ///< Word separator in text identifiers.
    unit::CpCount _sectionIndent{unit::CpCount{4}}; ///< Opening bracket column, or infinite for right alignment.
    unit::CpCount _sectionSuffix{unit::CpCount::infinite()};      ///< Trailing decoration width, or infinite for fill.
    bool _sectionStar{true};                                      ///< Emit the optional closing star on list sections.
    bool _spaceAfterOpen{true};                                   ///< Space after a section opening bracket.
    bool _spaceBeforeClose{true};                                 ///< Space before a section closing bracket.
    bool _spaceAroundDot{true};                                   ///< Spaces around section path separators.
    VectorStyle _vectorStyle{VectorStyle::Keep};                  ///< Vector layout policy.
    MatrixStyle _matrixStyle{MatrixStyle::Keep};                  ///< Matrix column alignment policy.
    unit::CpCount _spacesAfterStar{unit::CpCount{1}};             ///< Spaces after a list bullet.
    text::Char _separator{text::Char{U':'}};                      ///< Assignment separator, colon or equals.
    PositionMode _separatorPositionMode{PositionMode::Column};    ///< Placement mode for the assignment separator.
    unit::CpCount _separatorPosition{unit::CpCount{29}};          ///< Spaces or target column before the separator.
    PositionMode _valuePositionMode{PositionMode::Spaces};        ///< Placement mode for the first value.
    unit::CpCount _valuePosition{unit::CpCount{1}};               ///< Spaces or target column after the separator.
    IndentationStyle _indentationStyle{IndentationStyle::Spaces}; ///< Continuation indentation character.
    unit::CpCount _indentationCount{unit::CpCount{4}};            ///< Positive continuation indentation count.
};

}
