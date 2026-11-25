// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatStyle.hpp"

#include "../ConfError.hpp"
#include "../Name.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../text/StringTransformations.hpp"

#include <algorithm>

namespace erbsland::conf::syntax {

using namespace unit;
using namespace text;

using namespace text::literals;

FormatStyle::FormatStyle() :
    _abbreviations{"HTTP"_el, "HTTPS"_el, "FTP"_el, "FTPS"_el, "SSH"_el, "TLS"_el, "URL"_el, "VR"_el} {
}

auto FormatStyle::setAbbreviations(StringList value) -> FormatStyle & {
    auto normalized = StringList{};
    for (const auto &word : value) {
        try {
            const auto name = Name::createRegular(word);
            if (name.asText().contains("_"_el)) {
                throw err::ParameterFailure{"Abbreviations must be single regular words."_el, "value"_el};
            }
        } catch (const ConfError &) {
            throw err::ParameterFailure{"Abbreviations must be single regular words."_el, "value"_el};
        }
        const auto upper = toAsciiUppercase(word);
        if (std::ranges::find(normalized, upper) == normalized.end()) {
            normalized.append(upper);
        }
    }
    _abbreviations = std::move(normalized);
    return *this;
}

auto FormatStyle::setBooleanCase(WordCase value) noexcept -> FormatStyle & {
    _booleanCase = value;
    return *this;
}

auto FormatStyle::setBooleanForm(BooleanForm value) noexcept -> FormatStyle & {
    _booleanForm = value;
    return *this;
}

auto FormatStyle::setMaximumLineLength(CpCount value) noexcept -> FormatStyle & {
    _maximumLineLength = value;
    return *this;
}

auto FormatStyle::setSectionNameCase(WordCase value) noexcept -> FormatStyle & {
    _sectionNameCase = value;
    return *this;
}

auto FormatStyle::setSectionNameSeparator(WordSeparator value) noexcept -> FormatStyle & {
    _sectionNameSeparator = value;
    return *this;
}

auto FormatStyle::setValueNameCase(WordCase value) noexcept -> FormatStyle & {
    _valueNameCase = value;
    return *this;
}

auto FormatStyle::setValueNameSeparator(WordSeparator value) noexcept -> FormatStyle & {
    _valueNameSeparator = value;
    return *this;
}

auto FormatStyle::setTextNameCase(WordCase value) noexcept -> FormatStyle & {
    _textNameCase = value;
    return *this;
}

auto FormatStyle::setTextNameSeparator(WordSeparator value) noexcept -> FormatStyle & {
    _textNameSeparator = value;
    return *this;
}

auto FormatStyle::setTextIdentifierCase(WordCase value) noexcept -> FormatStyle & {
    _textIdentifierCase = value;
    return *this;
}

auto FormatStyle::setTextIdentifierSeparator(WordSeparator value) noexcept -> FormatStyle & {
    _textIdentifierSeparator = value;
    return *this;
}

auto FormatStyle::setSectionIndent(CpCount value) noexcept -> FormatStyle & {
    _sectionIndent = value;
    return *this;
}

auto FormatStyle::setSectionSuffix(CpCount value) noexcept -> FormatStyle & {
    _sectionSuffix = value;
    return *this;
}

auto FormatStyle::setSectionStar(bool value) noexcept -> FormatStyle & {
    _sectionStar = value;
    return *this;
}

auto FormatStyle::setSpaceAfterOpen(bool value) noexcept -> FormatStyle & {
    _spaceAfterOpen = value;
    return *this;
}

auto FormatStyle::setSpaceBeforeClose(bool value) noexcept -> FormatStyle & {
    _spaceBeforeClose = value;
    return *this;
}

auto FormatStyle::setSpaceAroundDot(bool value) noexcept -> FormatStyle & {
    _spaceAroundDot = value;
    return *this;
}

auto FormatStyle::setValueWrapping(const ValueWrapping value) noexcept -> FormatStyle & {
    _valueWrapping = value;
    return *this;
}

auto FormatStyle::setVectorStyle(VectorStyle value) noexcept -> FormatStyle & {
    _vectorStyle = value;
    return *this;
}

auto FormatStyle::setMatrixStyle(MatrixStyle value) noexcept -> FormatStyle & {
    _matrixStyle = value;
    return *this;
}

auto FormatStyle::setSpacesAfterStar(CpCount value) noexcept -> FormatStyle & {
    _spacesAfterStar = value;
    return *this;
}

auto FormatStyle::setSeparator(Char value) noexcept -> FormatStyle & {
    _separator = value;
    return *this;
}

auto FormatStyle::setSeparatorPositionMode(PositionMode value) noexcept -> FormatStyle & {
    _separatorPositionMode = value;
    return *this;
}

auto FormatStyle::setSeparatorPosition(CpCount value) noexcept -> FormatStyle & {
    _separatorPosition = value;
    return *this;
}

auto FormatStyle::setValuePositionMode(PositionMode value) noexcept -> FormatStyle & {
    _valuePositionMode = value;
    return *this;
}

auto FormatStyle::setValuePosition(CpCount value) noexcept -> FormatStyle & {
    _valuePosition = value;
    return *this;
}

auto FormatStyle::setIndentationStyle(IndentationStyle value) noexcept -> FormatStyle & {
    _indentationStyle = value;
    return *this;
}

auto FormatStyle::setIndentationCount(CpCount value) noexcept -> FormatStyle & {
    _indentationCount = value;
    return *this;
}

}
