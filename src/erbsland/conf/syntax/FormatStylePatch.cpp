// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatStylePatch.hpp"

namespace erbsland::conf::syntax {

using namespace unit;
using namespace text;

auto FormatStylePatch::setMaximumLineLength(CpCount value) noexcept -> FormatStylePatch & {
    _maximumLineLength = value;
    return *this;
}

auto FormatStylePatch::setSectionNameCase(WordCase value) noexcept -> FormatStylePatch & {
    _sectionNameCase = value;
    return *this;
}

auto FormatStylePatch::setSectionNameSeparator(WordSeparator value) noexcept -> FormatStylePatch & {
    _sectionNameSeparator = value;
    return *this;
}

auto FormatStylePatch::setValueNameCase(WordCase value) noexcept -> FormatStylePatch & {
    _valueNameCase = value;
    return *this;
}

auto FormatStylePatch::setValueNameSeparator(WordSeparator value) noexcept -> FormatStylePatch & {
    _valueNameSeparator = value;
    return *this;
}

auto FormatStylePatch::setTextNameCase(WordCase value) noexcept -> FormatStylePatch & {
    _textNameCase = value;
    return *this;
}

auto FormatStylePatch::setTextNameSeparator(WordSeparator value) noexcept -> FormatStylePatch & {
    _textNameSeparator = value;
    return *this;
}

auto FormatStylePatch::setTextIdentifierCase(WordCase value) noexcept -> FormatStylePatch & {
    _textIdentifierCase = value;
    return *this;
}

auto FormatStylePatch::setTextIdentifierSeparator(WordSeparator value) noexcept -> FormatStylePatch & {
    _textIdentifierSeparator = value;
    return *this;
}

auto FormatStylePatch::setSectionIndent(CpCount value) noexcept -> FormatStylePatch & {
    _sectionIndent = value;
    return *this;
}

auto FormatStylePatch::setSectionSuffix(CpCount value) noexcept -> FormatStylePatch & {
    _sectionSuffix = value;
    return *this;
}

auto FormatStylePatch::setSectionStar(bool value) noexcept -> FormatStylePatch & {
    _sectionStar = value;
    return *this;
}

auto FormatStylePatch::setSpaceAfterOpen(bool value) noexcept -> FormatStylePatch & {
    _spaceAfterOpen = value;
    return *this;
}

auto FormatStylePatch::setSpaceBeforeClose(bool value) noexcept -> FormatStylePatch & {
    _spaceBeforeClose = value;
    return *this;
}

auto FormatStylePatch::setSpaceAroundDot(bool value) noexcept -> FormatStylePatch & {
    _spaceAroundDot = value;
    return *this;
}

auto FormatStylePatch::setValueWrapping(const ValueWrapping value) noexcept -> FormatStylePatch & {
    _valueWrapping = value;
    return *this;
}

auto FormatStylePatch::setVectorStyle(VectorStyle value) noexcept -> FormatStylePatch & {
    _vectorStyle = value;
    return *this;
}

auto FormatStylePatch::setMatrixStyle(MatrixStyle value) noexcept -> FormatStylePatch & {
    _matrixStyle = value;
    return *this;
}

auto FormatStylePatch::setSpacesAfterStar(CpCount value) noexcept -> FormatStylePatch & {
    _spacesAfterStar = value;
    return *this;
}

auto FormatStylePatch::setSeparator(Char value) noexcept -> FormatStylePatch & {
    _separator = value;
    return *this;
}

auto FormatStylePatch::setSeparatorPositionMode(PositionMode value) noexcept -> FormatStylePatch & {
    _separatorPositionMode = value;
    return *this;
}

auto FormatStylePatch::setSeparatorPosition(CpCount value) noexcept -> FormatStylePatch & {
    _separatorPosition = value;
    return *this;
}

auto FormatStylePatch::setValuePositionMode(PositionMode value) noexcept -> FormatStylePatch & {
    _valuePositionMode = value;
    return *this;
}

auto FormatStylePatch::setValuePosition(CpCount value) noexcept -> FormatStylePatch & {
    _valuePosition = value;
    return *this;
}

auto FormatStylePatch::setIndentationStyle(IndentationStyle value) noexcept -> FormatStylePatch & {
    _indentationStyle = value;
    return *this;
}

auto FormatStylePatch::setIndentationCount(CpCount value) noexcept -> FormatStylePatch & {
    _indentationCount = value;
    return *this;
}

void FormatStylePatch::applyTo(FormatStyle &style) const noexcept {
    if (_valueWrapping) {
        style.setValueWrapping(*_valueWrapping);
    }
    if (_maximumLineLength) {
        style.setMaximumLineLength(*_maximumLineLength);
    }
    if (_sectionNameCase) {
        style.setSectionNameCase(*_sectionNameCase);
    }
    if (_sectionNameSeparator) {
        style.setSectionNameSeparator(*_sectionNameSeparator);
    }
    if (_valueNameCase) {
        style.setValueNameCase(*_valueNameCase);
    }
    if (_valueNameSeparator) {
        style.setValueNameSeparator(*_valueNameSeparator);
    }
    if (_textNameCase) {
        style.setTextNameCase(*_textNameCase);
    }
    if (_textNameSeparator) {
        style.setTextNameSeparator(*_textNameSeparator);
    }
    if (_textIdentifierCase) {
        style.setTextIdentifierCase(*_textIdentifierCase);
    }
    if (_textIdentifierSeparator) {
        style.setTextIdentifierSeparator(*_textIdentifierSeparator);
    }
    if (_sectionIndent) {
        style.setSectionIndent(*_sectionIndent);
    }
    if (_sectionSuffix) {
        style.setSectionSuffix(*_sectionSuffix);
    }
    if (_sectionStar) {
        style.setSectionStar(*_sectionStar);
    }
    if (_spaceAfterOpen) {
        style.setSpaceAfterOpen(*_spaceAfterOpen);
    }
    if (_spaceBeforeClose) {
        style.setSpaceBeforeClose(*_spaceBeforeClose);
    }
    if (_spaceAroundDot) {
        style.setSpaceAroundDot(*_spaceAroundDot);
    }
    if (_vectorStyle) {
        style.setVectorStyle(*_vectorStyle);
    }
    if (_matrixStyle) {
        style.setMatrixStyle(*_matrixStyle);
    }
    if (_spacesAfterStar) {
        style.setSpacesAfterStar(*_spacesAfterStar);
    }
    if (_separator) {
        style.setSeparator(*_separator);
    }
    if (_separatorPositionMode) {
        style.setSeparatorPositionMode(*_separatorPositionMode);
    }
    if (_separatorPosition) {
        style.setSeparatorPosition(*_separatorPosition);
    }
    if (_valuePositionMode) {
        style.setValuePositionMode(*_valuePositionMode);
    }
    if (_valuePosition) {
        style.setValuePosition(*_valuePosition);
    }
    if (_indentationStyle) {
        style.setIndentationStyle(*_indentationStyle);
    }
    if (_indentationCount) {
        style.setIndentationCount(*_indentationCount);
    }
}

}
