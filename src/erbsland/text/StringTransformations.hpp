// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Capitalization.hpp"
#include "Char.hpp"
#include "CharSet.hpp"

#include "impl/StringTraits.hpp"
#include "impl/StringTransformationTools.hpp"

#include <optional>

namespace erbsland::text {

/// Convert text to ASCII lowercase.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toAsciiLowercase(const T &text) -> impl::ReadOnlyStringFor<T> {
    return impl::ReadOnlyStringFor<T>{text}.transformed(Char::toAsciiLowercase);
}

/// Convert text to ASCII uppercase.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toAsciiUppercase(const T &text) -> impl::ReadOnlyStringFor<T> {
    return impl::ReadOnlyStringFor<T>{text}.transformed(Char::toAsciiUppercase);
}

/// Uppercase the first code point and lowercase the remainder using ASCII mappings.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toAsciiCapitalized(const T &text) -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, true>{text}.capitalized(Capitalization::Capitalized);
}

/// Apply the requested ASCII capitalization to the complete input.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param capitalization The capitalization to apply to the whole input as one word.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toAsciiCapitalization(const T &text, Capitalization capitalization) -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, true>{text}.capitalized(capitalization);
}

/// Convert text to Unicode lowercase.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toLowercase(const T &text) -> impl::ReadOnlyStringFor<T> {
    return impl::ReadOnlyStringFor<T>{text}.transformed(Char::toLowercase);
}

/// Convert text to Unicode uppercase.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toUppercase(const T &text) -> impl::ReadOnlyStringFor<T> {
    return impl::ReadOnlyStringFor<T>{text}.transformed(Char::toUppercase);
}

/// Uppercase the first code point and lowercase the remainder using Unicode mappings.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toCapitalized(const T &text) -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, false>{text}.capitalized(Capitalization::Capitalized);
}

/// Apply the requested Unicode capitalization to the complete input.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param capitalization The capitalization to apply to the whole input as one word.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toCapitalization(const T &text, Capitalization capitalization) -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, false>{text}.capitalized(capitalization);
}

/// Apply simple one-code-point Unicode case folding.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toCaseFolded(const T &text) -> impl::ReadOnlyStringFor<T> {
    return impl::ReadOnlyStringFor<T>{text}.transformed(Char::caseFolded);
}

/// Lowercase ASCII letters and replace spaces with underscores.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toIdentifierNormalized(const T &text) -> impl::ReadOnlyStringFor<T> {
    return impl::ReadOnlyStringFor<T>{text}.transformed(Char::toIdentifierNormalized);
}

/// Join prepared words into camel case using ASCII case mappings.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param wordSeparators Characters to remove between words; empty words are ignored.
/// @param capitalization Optional casing for every word; omitted preserves the default joining or splitting casing.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toAsciiCamelCaseFromWords(
    const T &text, const CharSet &wordSeparators, std::optional<Capitalization> capitalization = std::nullopt)
    -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, true>{text}.camelCaseFromWords(wordSeparators, capitalization);
}

/// Join words separated by ASCII whitespace into camel case.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param capitalization Optional casing for every word; omitted preserves the default joining or splitting casing.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toAsciiCamelCaseFromWords(const T &text, std::optional<Capitalization> capitalization = std::nullopt)
    -> impl::ReadOnlyStringFor<T> {
    return toAsciiCamelCaseFromWords(text, impl::StringTransformationTools<T, true>::wordSeparators(), capitalization);
}

/// Separate prepared camel-case words and acronyms using ASCII rules.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param wordSeparator A valid Unicode scalar to insert between words.
/// @param capitalization Optional casing for every word; omitted preserves the default joining or splitting casing.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toAsciiWordsFromCamelCase(
    const T &text, Char wordSeparator, std::optional<Capitalization> capitalization = std::nullopt)
    -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, true>{text}.wordsFromCamelCase(wordSeparator, capitalization);
}

/// Separate camel-case words with spaces using ASCII rules.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param capitalization Optional casing for every word; omitted preserves the default joining or splitting casing.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toAsciiWordsFromCamelCase(const T &text, std::optional<Capitalization> capitalization = std::nullopt)
    -> impl::ReadOnlyStringFor<T> {
    return toAsciiWordsFromCamelCase(text, Char{U' '}, capitalization);
}

/// Trim surrounding ASCII whitespace and collapse each internal run to one separator.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param wordSeparator A valid Unicode scalar to replace each internal whitespace run.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toAsciiWhitespaceSimplified(const T &text, Char wordSeparator = U' ') -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, true>{text}.whitespaceSimplified(wordSeparator);
}

/// Join prepared words into camel case using Unicode case mappings.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param wordSeparators Characters to remove between words; empty words are ignored.
/// @param capitalization Optional casing for every word; omitted preserves the default joining or splitting casing.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toCamelCaseFromWords(
    const T &text, const CharSet &wordSeparators, std::optional<Capitalization> capitalization = std::nullopt)
    -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, false>{text}.camelCaseFromWords(wordSeparators, capitalization);
}

/// Join words separated by Unicode whitespace into camel case.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param capitalization Optional casing for every word; omitted preserves the default joining or splitting casing.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toCamelCaseFromWords(const T &text, std::optional<Capitalization> capitalization = std::nullopt)
    -> impl::ReadOnlyStringFor<T> {
    return toCamelCaseFromWords(text, impl::StringTransformationTools<T, false>::wordSeparators(), capitalization);
}

/// Separate prepared camel-case words and acronyms using Unicode rules.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param wordSeparator A valid Unicode scalar to insert between words.
/// @param capitalization Optional casing for every word; omitted preserves the default joining or splitting casing.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toWordsFromCamelCase(
    const T &text, Char wordSeparator, std::optional<Capitalization> capitalization = std::nullopt)
    -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, false>{text}.wordsFromCamelCase(wordSeparator, capitalization);
}

/// Separate camel-case words with spaces using Unicode rules.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param capitalization Optional casing for every word; omitted preserves the default joining or splitting casing.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toWordsFromCamelCase(const T &text, std::optional<Capitalization> capitalization = std::nullopt)
    -> impl::ReadOnlyStringFor<T> {
    return toWordsFromCamelCase(text, Char{U' '}, capitalization);
}

/// Trim surrounding Unicode whitespace and collapse each internal run to one separator.
/// @seedoc{/reference/text/strings}
/// @tparam T A concrete Core UTF-8, UTF-16, or UTF-32 string or editor.
/// @param text The input text; its value is not modified.
/// @param wordSeparator A valid Unicode scalar to replace each internal whitespace run.
/// @return A read-only string of the same encoding, sharing source storage when unchanged.
/// @usesunidb{Uses generated Unicode character classifications or simple case mappings.}
/// @tested{StringTransformationsTest}
template <impl::AnyStringOrStringEditorType T>
[[nodiscard]] auto toWhitespaceSimplified(const T &text, Char wordSeparator = U' ') -> impl::ReadOnlyStringFor<T> {
    return impl::StringTransformationTools<T, false>{text}.whitespaceSimplified(wordSeparator);
}

}
