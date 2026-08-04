// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DataSection.hpp"
#include "OperationData.hpp"
#include "OperationModifier.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../unit/ColumnIndex.hpp"

#include <cstdint>
#include <type_traits>
#include <variant>
#include <vector>

namespace erbsland::re::impl {

/// Represents one token in a regular-expression diagnostic assembler source.
/// @tested{AssemblerTokenTest}
class AssemblerToken {
public:
    /// Identifies the lexical kind of an assembler token.
    enum Type : uint8_t {
        Integer,    ///< A positive integer. `0`, `123`, or `0x4000`
        Text,       ///< Double quoted text `"text"`.
        Char,       ///< Single quoted character `'a'`.
        Boolean,    ///< A boolean `true`, `false`
        Modifier,   ///< A modifier: `CI`, `NOT`, `START`, `STOP`
        Operation,  ///< An operator, like `JUMP`, `NONE`, `SPLIT`...
        Label,      ///< A label. `label:` (at line start) or `%label` (as argument).
        Identifier, ///< An identifier. `&identifier`.
        Offset,     ///< A hexadecimal integer. `$002f`, `$a2`, or `$1234abcd`
        Comma,      ///< A comma to separate arguments. `,` value = integer 0
        Minus,      ///< A minus sign. `-` value = integer 0
        Command,    ///< A command. `.command`
        Comment,    ///< A comment. `; comment`
    };

    /// Defines the value stored by a token.
    using Value = std::variant<text::String, uint32_t, bool, impl::Operation, OperationModifier>;

public:
    /// Create a token with a type, value, and source column.
    constexpr AssemblerToken(const Type type, Value value, const unit::ColumnIndex column) noexcept :
        _type{type}, _value{std::move(value)}, _column{column} {}

    // defaults
    ~AssemblerToken() = default;
    AssemblerToken(const AssemblerToken &) = default;
    AssemblerToken(AssemblerToken &&) = default;
    auto operator=(const AssemblerToken &) -> AssemblerToken & = default;
    auto operator=(AssemblerToken &&) -> AssemblerToken & = default;

public: // accessors and tests.
    /// Get the token type.
    [[nodiscard]] auto type() const noexcept -> Type { return _type; }
    /// Get the token value.
    [[nodiscard]] auto value() const noexcept -> const Value & { return _value; }
    /// Get the token's source column.
    [[nodiscard]] auto column() const noexcept -> unit::ColumnIndex { return _column; }

    /// Get the display name of the token type.
    [[nodiscard]] auto typeName() const noexcept -> text::String {
        using namespace text::literals;
        switch (_type) {
        case Integer:
            return "Integer"_el;
        case Text:
            return "Text"_el;
        case Char:
            return "Char"_el;
        case Boolean:
            return "Boolean"_el;
        case Modifier:
            return "Modifier"_el;
        case Operation:
            return "Operation"_el;
        case Label:
            return "Label"_el;
        case Identifier:
            return "Identifier"_el;
        case Offset:
            return "Offset"_el;
        case Comma:
            return "Comma"_el;
        case Minus:
            return "Minus"_el;
        case Command:
            return "Command"_el;
        case Comment:
            return "Comment"_el;
        }
        return "<unknown>"_el;
    }

    /// Format this token for diagnostics.
    [[nodiscard]] auto toString() const -> text::String {
        using namespace text::literals;
        auto valueString = text::String{};
        std::visit(
            [&](const auto &v) -> void {
                using ValueType = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<ValueType, text::String>) {
                    valueString = text::StringFormat{"\"{}\""}.build(v);
                } else if constexpr (std::is_same_v<ValueType, std::uint32_t>) {
                    valueString = text::String::fromInteger(v);
                } else if constexpr (std::is_same_v<ValueType, bool>) {
                    valueString = text::String::fromBoolean(v);
                } else if constexpr (std::is_same_v<ValueType, impl::Operation>) {
                    valueString = impl::toString(v);
                } else if constexpr (std::is_same_v<ValueType, OperationModifier>) {
                    valueString = impl::toString(v);
                } else {
                    valueString = "<unprintable>"_el;
                }
            },
            _value);
        return text::StringFormat{"col={} type={} value={}"}.build(_column, typeName(), valueString);
    }

    /// Test whether this is an operation token.
    [[nodiscard]] auto isOperation() const noexcept -> bool { return _type == Operation; }
    /// Test whether this is a label token.
    [[nodiscard]] auto isLabel() const noexcept -> bool { return _type == Label; }
    /// Test whether this is a comma token.
    [[nodiscard]] auto isComma() const noexcept -> bool { return _type == Comma; }
    /// Test whether this is a minus token.
    [[nodiscard]] auto isMinus() const noexcept -> bool { return _type == Minus; }
    /// Test whether this is a command token.
    [[nodiscard]] auto isCommand() const noexcept -> bool { return _type == Command; }
    /// Test whether this is an integer token.
    [[nodiscard]] auto isInteger() const noexcept -> bool { return _type == Integer; }
    /// Test whether this is a text token.
    [[nodiscard]] auto isText() const noexcept -> bool { return _type == Text; }
    /// Test whether this is a character token.
    [[nodiscard]] auto isChar() const noexcept -> bool { return _type == Char; }
    /// Test whether this is a Boolean token.
    [[nodiscard]] auto isBoolean() const noexcept -> bool { return _type == Boolean; }
    /// Test whether this is a modifier token.
    [[nodiscard]] auto isModifier() const noexcept -> bool { return _type == Modifier; }
    /// Test whether this is an offset token.
    [[nodiscard]] auto isOffset() const noexcept -> bool { return _type == Offset; }
    /// Test whether this is an identifier token.
    [[nodiscard]] auto isIdentifier() const noexcept -> bool { return _type == Identifier; }
    /// Test whether this token can be used as an operation argument.
    [[nodiscard]] auto isArgument() const noexcept -> bool {
        return _type == Integer || _type == Text || _type == Char || _type == Boolean || _type == Label ||
            _type == Identifier || _type == Offset;
    }

    /// Get the token value as text.
    [[nodiscard]] auto getText() const noexcept -> const text::String & { return std::get<text::String>(_value); }
    /// Get the token value as an integer.
    [[nodiscard]] auto getInteger() const noexcept -> uint32_t { return std::get<std::uint32_t>(_value); }
    /// Get the token value as a Boolean.
    [[nodiscard]] auto getBoolean() const noexcept -> bool { return std::get<bool>(_value); }
    /// Get the token value as an operation.
    [[nodiscard]] auto getOperation() const noexcept -> impl::Operation { return std::get<impl::Operation>(_value); }
    /// Get the token value as an operation modifier.
    [[nodiscard]] auto getModifier() const noexcept -> OperationModifier { return std::get<OperationModifier>(_value); }

private:
    Type _type;                ///< The lexical token type.
    Value _value;              ///< The token payload.
    unit::ColumnIndex _column; ///< The source column of the token.
};

/// Defines a sequence of assembler tokens.
using AssemblerTokens = std::vector<AssemblerToken>;

}
