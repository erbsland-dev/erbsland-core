// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TextPlaceholderParameterKey.hpp"

#include "../../../text/CharSet.hpp"
#include "../../../text/EscapeAmount.hpp"
#include "../../../text/EscapeFormat.hpp"
#include "../../../text/String.hpp"
#include "../../../text/StringSide.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"

#include <cassert>
#include <initializer_list>
#include <map>
#include <optional>
#include <utility>
#include <variant>

namespace erbsland::conf::impl::placeholder {

/// Parsed parameters and behavior for the built-in text filters.
/// @tested{ParserPlaceholderTest}
class TextPlaceholderParameters final {
public:
    /// A side selection, including both sides.
    enum class Side {
        Both,
        Front,
        Back,
    };

    using Key = TextPlaceholderParameterKey;

public:
    /// Parse a built-in filter parameter list.
    [[nodiscard]] static auto parse(const text::String &parameter) -> TextPlaceholderParameters;

    /// Test if a parameter is present.
    [[nodiscard]] auto has(Key key) const noexcept -> bool { return _values.contains(key); }
    /// Get the number of parameters.
    [[nodiscard]] auto count() const noexcept -> std::size_t { return _values.size(); }

    /// Access the optional named start index.
    [[nodiscard]] auto start() const -> std::optional<unit::CpIndex> { return value<unit::CpIndex>(Key::Start); }
    /// Access the optional named length.
    [[nodiscard]] auto length() const -> std::optional<unit::CpLength> { return value<unit::CpLength>(Key::Length); }
    /// Access the optional side.
    [[nodiscard]] auto side() const -> std::optional<Side> { return value<Side>(Key::Side); }
    /// Access the optional character set.
    [[nodiscard]] auto chars() const -> std::optional<text::CharSet> { return value<text::CharSet>(Key::Chars); }
    /// Access the optional text.
    [[nodiscard]] auto textValue() const -> std::optional<text::String> { return value<text::String>(Key::Text); }
    /// Access the optional replacement text.
    [[nodiscard]] auto replacement() const -> std::optional<text::String> {
        return value<text::String>(Key::Replacement);
    }
    /// Access the optional escape format.
    [[nodiscard]] auto format() const -> std::optional<text::EscapeFormat> {
        return value<text::EscapeFormat>(Key::Format);
    }
    /// Access the optional escape amount.
    [[nodiscard]] auto amount() const -> std::optional<text::EscapeAmount> {
        return value<text::EscapeAmount>(Key::Amount);
    }
    /// Access the optional contains condition.
    [[nodiscard]] auto containsText() const -> std::optional<text::String> {
        return value<text::String>(Key::Contains);
    }
    /// Access the optional equal-length condition.
    [[nodiscard]] auto lengthEqual() const -> std::optional<unit::CpLength> {
        return value<unit::CpLength>(Key::LengthEqual);
    }
    /// Access the optional greater-length condition.
    [[nodiscard]] auto lengthGreater() const -> std::optional<unit::CpLength> {
        return value<unit::CpLength>(Key::LengthGreater);
    }
    /// Access the optional less-length condition.
    [[nodiscard]] auto lengthLess() const -> std::optional<unit::CpLength> {
        return value<unit::CpLength>(Key::LengthLess);
    }
    /// Test if the empty condition is present.
    [[nodiscard]] auto empty() const noexcept -> bool { return has(Key::Empty); }
    /// Access the optional true result.
    [[nodiscard]] auto thenText() const -> std::optional<text::String> { return value<text::String>(Key::Then); }
    /// Access the optional false result.
    [[nodiscard]] auto elseText() const -> std::optional<text::String> { return value<text::String>(Key::Else); }
    /// Access the optional positional parameter.
    [[nodiscard]] auto positional() const -> std::optional<text::String> {
        return value<text::String>(Key::Positional);
    }

    /// Apply the `safe` filter.
    [[nodiscard]] auto applySafe(const text::String &input) const -> text::String;
    /// Apply the `trim` filter.
    [[nodiscard]] auto applyTrim(const text::String &input) const -> text::String;
    /// Apply the `slice` filter.
    [[nodiscard]] auto applySlice(const text::String &input) const -> text::String;
    /// Apply the `remove` filter.
    [[nodiscard]] auto applyRemove(const text::String &input) const -> text::String;
    /// Apply the `replace` filter.
    [[nodiscard]] auto applyReplace(const text::String &input) const -> text::String;
    /// Apply the `escape` filter.
    [[nodiscard]] auto applyEscape(const text::String &input) const -> text::String;
    /// Apply the `if` filter.
    [[nodiscard]] auto applyIf(const text::String &input) const -> text::String;
    /// Apply the `error_if` filter.
    [[nodiscard]] auto applyErrorIf(const text::String &input) const -> text::String;

private:
    using Value = std::variant<
        unit::CpIndex,
        unit::CpLength,
        Side,
        text::CharSet,
        text::String,
        text::EscapeFormat,
        text::EscapeAmount,
        bool>;
    using ValueMap = std::map<Key, Value>;

    /// Store a parsed parameter value.
    void set(Key key, Value value) { _values.insert_or_assign(key, std::move(value)); }

    /// Access an optional typed value.
    template <typename T>
    [[nodiscard]] auto value(const Key key) const -> std::optional<T> {
        const auto position = _values.find(key);
        if (position == _values.end()) {
            return {};
        }
        const auto *result = std::get_if<T>(&position->second);
        assert(result != nullptr);
        return *result;
    }

    /// Test if all parameters are in the allowed set.
    [[nodiscard]] auto hasOnly(std::initializer_list<Key> allowedKeys) const noexcept -> bool;
    /// Count configured conditions.
    [[nodiscard]] auto conditionCount() const noexcept -> std::size_t;
    /// Evaluate the single configured condition.
    [[nodiscard]] auto evaluateCondition(const text::String &input) const -> bool;

    /// Parse a code-point index.
    [[nodiscard]] static auto parseIndex(const text::String &value) -> unit::CpIndex;
    /// Parse a code-point length.
    [[nodiscard]] static auto parseLength(const text::String &value) -> unit::CpLength;
    /// Parse a string side.
    [[nodiscard]] static auto parseSide(const text::String &value) -> Side;
    /// Convert a side into the string API representation.
    [[nodiscard]] static auto stringSide(Side value) noexcept -> std::optional<text::StringSide>;
    /// Parse a bracketed character-set pattern.
    [[nodiscard]] static auto parseChars(const text::String &value) -> text::CharSet;
    /// Parse an escape format.
    [[nodiscard]] static auto parseEscapeFormat(const text::String &value) -> text::EscapeFormat;
    /// Parse an escape amount.
    [[nodiscard]] static auto parseEscapeAmount(const text::String &value) -> text::EscapeAmount;
    /// Verify a parameter constraint.
    static void verify(bool condition, text::String message);

private:
    ValueMap _values;
};

}
