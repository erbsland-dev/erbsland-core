// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

namespace erbsland::text::impl {

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::capitalized(const Capitalization capitalization) -> String {
    if (capitalization == Capitalization::Lowercase) {
        return _source.transformed(lowercase);
    }
    if (capitalization == Capitalization::Uppercase) {
        return _source.transformed(uppercase);
    }
    for (const auto character : _source) {
        write(character, capitalize(character, capitalization, _position.isZero()));
    }
    return result();
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::camelCaseFromWords(
    const CharSet &separators, const std::optional<Capitalization> capitalization) -> String {
    auto first = true;
    auto hasWord = false;
    for (const auto character : _source) {
        if (separators.contains(character)) {
            write(character, Char::noCodePoint());
            first = true;
            continue;
        }
        auto mapped = character;
        if (capitalization) {
            mapped = capitalize(character, *capitalization, first);
        } else if (first && hasWord) {
            mapped = uppercase(character);
        }
        write(character, mapped);
        first = false;
        hasWord = true;
    }
    return result();
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::wordsFromCamelCase(
    const Char separator, const std::optional<Capitalization> capitalization) -> String {
    auto previous = Char::noCodePoint();
    auto current = _source.begin();
    const auto end = _source.end();
    while (current != end) {
        const auto character = *current;
        ++current;
        auto next = Char::noCodePoint();
        if (current != end) {
            next = *current;
        }
        const auto boundary = isBoundary(previous, character, next);
        if (boundary) {
            insert(separator);
        }
        auto mapped = character;
        if (capitalization) {
            mapped = capitalize(character, *capitalization, _position.isZero() || boundary);
        }
        write(character, mapped);
        previous = character;
    }
    return result();
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::whitespaceSimplified(const Char separator) -> String {
    const auto &separators = wordSeparators();
    auto current = _source.begin();
    const auto end = _source.end();
    auto hasWord = false;
    while (current != end) {
        const auto character = *current;
        ++current;
        if (!separators.contains(character)) {
            write(character, character);
            hasWord = true;
            continue;
        }
        auto count = unit::CpCount::one();
        while (current != end && separators.contains(*current)) {
            ++current;
            ++count;
        }
        if (hasWord && current != end) {
            write(character, separator);
            --count;
        }
        skip(count);
    }
    return result();
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::wordSeparators() -> const CharSet & {
    if constexpr (tAscii) {
        return asciiTransformationSeparators();
    } else {
        return unicodeTransformationSeparators();
    }
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::lowercase(const Char character) noexcept -> Char {
    if constexpr (tAscii) {
        return character.toAsciiLowercase();
    } else {
        return character.toLowercase();
    }
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::uppercase(const Char character) noexcept -> Char {
    if constexpr (tAscii) {
        return character.toAsciiUppercase();
    } else {
        return character.toUppercase();
    }
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::capitalize(
    const Char character, const Capitalization capitalization, const bool first) noexcept -> Char {
    if (capitalization == Capitalization::Uppercase || (capitalization == Capitalization::Capitalized && first)) {
        return uppercase(character);
    }
    return lowercase(character);
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::isLowercase(const Char character) noexcept -> bool {
    if constexpr (tAscii) {
        return character.isAsciiLowercaseLetter();
    } else {
        return character.isCategory(UnicodeCategory::LowercaseLetter);
    }
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::isUppercase(const Char character) noexcept -> bool {
    if constexpr (tAscii) {
        return character.isAsciiUppercaseLetter();
    } else {
        const auto category = character.category();
        return category == UnicodeCategory::UppercaseLetter || category == UnicodeCategory::TitlecaseLetter;
    }
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::isDigit(const Char character) noexcept -> bool {
    if constexpr (tAscii) {
        return character.isAsciiDigit();
    } else {
        return character.isCategory(UnicodeCategory::DecimalNumber);
    }
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::isBoundary(
    const Char previous, const Char current, const Char next) noexcept -> bool {
    return isUppercase(current) &&
        (isLowercase(previous) || isDigit(previous) || (isUppercase(previous) && isLowercase(next)));
}

template <AnyStringOrStringEditorType tString, bool tAscii>
void StringTransformationTools<tString, tAscii>::beginResult() {
    if (_result) {
        return;
    }
    _result.emplace();
    if constexpr (std::same_as<String, U8String>) {
        if (_source.isSensitive()) {
            _result->markAsSensitive();
        }
    }
    _result->append(_source.slice(unit::CpRange{unit::CpIndex::zero(), _position}));
}

template <AnyStringOrStringEditorType tString, bool tAscii>
void StringTransformationTools<tString, tAscii>::write(const Char original, const Char mapped) {
    if (original != mapped) {
        beginResult();
    }
    if (_result && mapped.isValidUnicode()) {
        _result->append(mapped);
    }
    ++_position;
}

template <AnyStringOrStringEditorType tString, bool tAscii>
void StringTransformationTools<tString, tAscii>::skip(const unit::CpCount count) {
    if (!count.isZero()) {
        beginResult();
        _position += count;
    }
}

template <AnyStringOrStringEditorType tString, bool tAscii>
void StringTransformationTools<tString, tAscii>::insert(const Char character) {
    beginResult();
    _result->append(character);
}

template <AnyStringOrStringEditorType tString, bool tAscii>
auto StringTransformationTools<tString, tAscii>::result() const -> String {
    if (_result) {
        return String{*_result};
    }
    return _source;
}

}
