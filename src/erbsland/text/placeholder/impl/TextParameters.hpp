// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TextParameterKey.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../CharSet.hpp"
#include "../../EscapeAmount.hpp"
#include "../../EscapeFormat.hpp"
#include "../../String.hpp"
#include "../../StringSide.hpp"

#include <cassert>
#include <initializer_list>
#include <map>
#include <optional>
#include <utility>
#include <variant>

namespace erbsland::text::placeholder::impl {

/// Parsed parameters and behavior for the built-in text filters.
/// @tested{ParserPlaceholderTest}
class TextParameters final {
public:
    /// A side selection, including both sides.
    enum class Side {
        Both,
        Front,
        Back,
    };

    using Key = TextParameterKey;

public:
    /// Parse a built-in filter parameter list.
    [[nodiscard]] static auto parse(const String &parameter) -> TextParameters;
    /// Validate parameters for one built-in filter without evaluating a value.
    void validateFor(const String &filterName) const;

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
    [[nodiscard]] auto chars() const -> std::optional<CharSet> { return value<CharSet>(Key::Chars); }
    /// Access the optional text.
    [[nodiscard]] auto textValue() const -> std::optional<String> { return value<String>(Key::Text); }
    /// Access the optional replacement text.
    [[nodiscard]] auto replacement() const -> std::optional<String> { return value<String>(Key::Replacement); }
    /// Access the optional escape format.
    [[nodiscard]] auto format() const -> std::optional<EscapeFormat> { return value<EscapeFormat>(Key::Format); }
    /// Access the optional escape amount.
    [[nodiscard]] auto amount() const -> std::optional<EscapeAmount> { return value<EscapeAmount>(Key::Amount); }
    /// Access the optional contains condition.
    [[nodiscard]] auto containsText() const -> std::optional<String> { return value<String>(Key::Contains); }
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
    [[nodiscard]] auto thenText() const -> std::optional<String> { return value<String>(Key::Then); }
    /// Access the optional false result.
    [[nodiscard]] auto elseText() const -> std::optional<String> { return value<String>(Key::Else); }
    /// Access the optional positional parameter.
    [[nodiscard]] auto positional() const -> std::optional<String> { return value<String>(Key::Positional); }

    /// Apply the `safe` filter.
    [[nodiscard]] auto applySafe(const String &input) const -> String;
    /// Apply the `trim` filter.
    [[nodiscard]] auto applyTrim(const String &input) const -> String;
    /// Apply the `slice` filter.
    [[nodiscard]] auto applySlice(const String &input) const -> String;
    /// Apply the `remove` filter.
    [[nodiscard]] auto applyRemove(const String &input) const -> String;
    /// Apply the `replace` filter.
    [[nodiscard]] auto applyReplace(const String &input) const -> String;
    /// Apply the `escape` filter.
    [[nodiscard]] auto applyEscape(const String &input) const -> String;
    /// Apply the `if` filter.
    [[nodiscard]] auto applyIf(const String &input) const -> String;
    /// Apply the `error_if` filter.
    [[nodiscard]] auto applyErrorIf(const String &input) const -> String;

private:
    using Value = std::variant<unit::CpIndex, unit::CpLength, Side, CharSet, String, EscapeFormat, EscapeAmount, bool>;
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
    [[nodiscard]] auto evaluateCondition(const String &input) const -> bool;

    /// Parse a code-point index.
    [[nodiscard]] static auto parseIndex(const String &value) -> unit::CpIndex;
    /// Parse a code-point length.
    [[nodiscard]] static auto parseLength(const String &value) -> unit::CpLength;
    /// Parse a string side.
    [[nodiscard]] static auto parseSide(const String &value) -> Side;
    /// Convert a side into the string API representation.
    [[nodiscard]] static auto stringSide(Side value) noexcept -> std::optional<StringSide>;
    /// Parse a bracketed character-set pattern.
    [[nodiscard]] static auto parseChars(const String &value) -> CharSet;
    /// Parse an escape format.
    [[nodiscard]] static auto parseEscapeFormat(const String &value) -> EscapeFormat;
    /// Parse an escape amount.
    [[nodiscard]] static auto parseEscapeAmount(const String &value) -> EscapeAmount;
    /// Verify a parameter constraint.
    static void verify(bool condition, String message);

private:
    ValueMap _values;
};

}
