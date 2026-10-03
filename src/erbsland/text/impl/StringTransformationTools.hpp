// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringKeyTraits.hpp"
#include "StringTraits.hpp"
#include "StringTransformationSeparators.hpp"

#include "../Capitalization.hpp"
#include "../Char.hpp"
#include "../CharSet.hpp"
#include "../u16/U16String.hpp"
#include "../u16/U16StringConstIterator.hpp"
#include "../u16/U16StringEditor.hpp"
#include "../u32/U32String.hpp"
#include "../u32/U32StringConstIterator.hpp"
#include "../u32/U32StringEditor.hpp"
#include "../u8/U8String.hpp"
#include "../u8/U8StringConstIterator.hpp"
#include "../u8/U8StringEditor.hpp"
#include "../UnicodeCategory.hpp"

#include "../../unit/CpCount.hpp"
#include "../../unit/CpIndex.hpp"
#include "../../unit/CpRange.hpp"

#include <concepts>
#include <optional>

namespace erbsland::text::impl {

/// Transform decoded text, allocating result storage only at the first change.
/// @tparam tString The concrete input string or editor type.
/// @tparam tAscii Whether classification and case mapping use only ASCII rules.
/// @tested{StringTransformationsTest}
template <AnyStringOrStringEditorType tString, bool tAscii>
class StringTransformationTools final {
private:
    using String = ReadOnlyStringFor<tString>;
    using Editor = typename StringTypes<String>::Editor;

public:
    /// Retain a shared read-only view of the input.
    /// @param text The input string or editor.
    explicit StringTransformationTools(const tString &text) : _source{text} {}

public: // transformations
    /// Apply one capitalization to the complete input.
    /// @param capitalization The requested capitalization.
    /// @return The resulting read-only string.
    [[nodiscard]] auto capitalized(Capitalization capitalization) -> String;
    /// Join nonempty words, preserving their case unless explicitly requested otherwise.
    /// @param separators The characters separating words.
    /// @param capitalization Optional capitalization applied independently to every word.
    /// @return The joined words.
    [[nodiscard]] auto camelCaseFromWords(const CharSet &separators, std::optional<Capitalization> capitalization)
        -> String;
    /// Separate camel-case words and acronyms.
    /// @param separator The valid Unicode scalar inserted at each boundary.
    /// @param capitalization Optional capitalization applied independently to every word.
    /// @return The separated words.
    [[nodiscard]] auto wordsFromCamelCase(Char separator, std::optional<Capitalization> capitalization) -> String;
    /// Trim and collapse runs of whitespace.
    /// @param separator The valid Unicode scalar replacing internal whitespace runs.
    /// @return The simplified text.
    [[nodiscard]] auto whitespaceSimplified(Char separator) -> String;

public: // shared defaults
    /// Get the shared whitespace set for the selected classification rules.
    /// @return The ASCII or Unicode whitespace set.
    [[nodiscard]] static auto wordSeparators() -> const CharSet &;

private:
    /// Map one code point to lowercase with the selected rules.
    [[nodiscard]] static auto lowercase(Char character) noexcept -> Char;
    /// Map one code point to uppercase with the selected rules.
    [[nodiscard]] static auto uppercase(Char character) noexcept -> Char;
    /// Apply the selected capitalization, accounting for the first code point of a word.
    [[nodiscard]] static auto capitalize(Char character, Capitalization capitalization, bool first) noexcept -> Char;
    /// Test for a lowercase letter with the selected rules.
    [[nodiscard]] static auto isLowercase(Char character) noexcept -> bool;
    /// Test for an uppercase or Unicode titlecase letter with the selected rules.
    [[nodiscard]] static auto isUppercase(Char character) noexcept -> bool;
    /// Test for a decimal digit with the selected rules.
    [[nodiscard]] static auto isDigit(Char character) noexcept -> bool;
    /// Test the neighboring original code points for a camel-case boundary.
    [[nodiscard]] static auto isBoundary(Char previous, Char current, Char next) noexcept -> bool;
    /// Allocate an editor containing the unchanged source prefix, at most once.
    void beginResult();
    /// Consume one source code point, optionally replacing or removing it.
    void write(Char original, Char mapped);
    /// Remove a run of source code points.
    void skip(unit::CpCount count);
    /// Insert a character before the current source position.
    void insert(Char character);
    /// Return the built result or the unchanged shared source.
    [[nodiscard]] auto result() const -> String;

private:
    String _source;                ///< Shared immutable source.
    std::optional<Editor> _result; ///< Result storage created only when needed.
    unit::CpIndex _position{};     ///< Number of consumed source code points.
};

}

#include "StringTransformationTools.tpp"
