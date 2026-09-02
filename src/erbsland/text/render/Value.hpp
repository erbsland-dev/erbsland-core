// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Value_fwd.hpp"
#include "ValueType.hpp"

#include "impl/ValueData_fwd.hpp"

#include "../String.hpp"
#include "../StringLiteral.hpp"
#include "../StringMap.hpp"

#include "../../err/ParameterError.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../unit/ItemIndex.hpp"
#include "../../util/List_fwd.hpp"

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace erbsland::text::render {

/// An immutable value exposed to the layout renderer.
/// @seedoc{/reference/text/documents_and_rendering}
/// @tested{RenderValueTest}
class Value final {
public:
    using Storage = std::shared_ptr<impl::ValueData>;

public:
    /// Create a null value.
    Value() noexcept = default;
    /// Create a boolean value.
    Value(bool value); // NOLINT(*-explicit-constructor)
    /// Create a text value.
    Value(String text); // NOLINT(*-explicit-constructor)
    /// Create a text value from an Erbsland Core string literal.
    Value(StringLiteral text) : Value{String{text}} {} // NOLINT(*-explicit-constructor)
    /// Create an integer value.
    Value(int64_t value); // NOLINT(*-explicit-constructor)
    /// Create an integer value from another integral type.
    /// @throws err::ParameterError If `value` does not fit into `int64_t`.
    template <std::integral T>
        requires(!std::same_as<std::remove_cv_t<T>, bool> && !std::same_as<std::remove_cv_t<T>, int64_t>)
    Value(const T value) : Value{checkedInteger(value)} {} // NOLINT(*-explicit-constructor)
    /// Create a float value.
    Value(double value); // NOLINT(*-explicit-constructor)
    /// Create a floating-point value from another floating-point type.
    template <std::floating_point T>
        requires(!std::same_as<std::remove_cv_t<T>, double>)
    Value(const T value) : Value{static_cast<double>(value)} {} // NOLINT(*-explicit-constructor)
    /// Create a list value.
    Value(const util::List<Value> &value); // NOLINT(*-explicit-constructor)
    /// Create a map value.
    Value(const StringMap<Value> &value); // NOLINT(*-explicit-constructor)
    /// Create a lazily evaluated callback value.
    Value(ValueCallbackFn value); // NOLINT(*-explicit-constructor)

    // defaults
    ~Value() = default;
    Value(const Value &) noexcept = default;
    Value(Value &&) noexcept = default;
    auto operator=(const Value &) noexcept -> Value & = default;
    auto operator=(Value &&) noexcept -> Value & = default;

public: // tests and accessors
    /// The type of the value.
    [[nodiscard]] auto type() const noexcept -> ValueType;
    /// Test if this is a null value.
    [[nodiscard]] auto isNull() const noexcept -> bool;
    /// Test if this is a boolean value.
    [[nodiscard]] auto isBoolean() const noexcept -> bool;
    /// Test if this is an integer value.
    [[nodiscard]] auto isInteger() const noexcept -> bool;
    /// Test if this is a floating-point value.
    [[nodiscard]] auto isFloat() const noexcept -> bool;
    /// Test if this is a text value.
    [[nodiscard]] auto isText() const noexcept -> bool;
    /// Test if this is a list value.
    [[nodiscard]] auto isList() const noexcept -> bool;
    /// Test if this is a map.
    [[nodiscard]] auto isMap() const noexcept -> bool;
    /// Test if this is a callback.
    [[nodiscard]] auto isCallback() const noexcept -> bool;
    /// Get the number of list or map items, or zero for other types.
    [[nodiscard]] auto itemCount() const noexcept -> unit::ItemCount;
    /// Test this value using the renderer truth rules.
    [[nodiscard]] auto isTruthy() const noexcept -> bool;

public: // scalar access
    /// Get the stored boolean.
    /// @throws err::LogicError If this is not a boolean value.
    [[nodiscard]] auto asBoolean() const -> bool;
    /// Get the stored integer.
    /// @throws err::LogicError If this is not an integer value.
    [[nodiscard]] auto asInteger() const -> int64_t;
    /// Get the stored floating-point number.
    /// @throws err::LogicError If this is not a floating-point value.
    [[nodiscard]] auto asFloat() const -> double;
    /// Get the stored text.
    /// @throws err::LogicError If this is not a text value.
    [[nodiscard]] auto asText() const -> String;
    /// Get the stored list.
    /// @throws err::LogicError If this is not a list value.
    [[nodiscard]] auto asList() const -> const ValueList &;
    /// Get the stored map.
    /// @throws err::LogicError If this is not a map value.
    [[nodiscard]] auto asMap() const -> const ValueMap &;

public: // child access
    /// Get a list item, or null if this is not a list or the index is invalid.
    [[nodiscard]] auto get(unit::ItemIndex index) const -> Value;
    /// Get a map item, or null if this is not a map or the name is missing.
    [[nodiscard]] auto get(const String &name) const -> Value;
    /// Evaluate one callback layer, or return this value unchanged when it is not a callback.
    [[nodiscard]] auto evaluate() const -> Value;

public: // conversion
    /// Convert this value into a string.
    /// This convert all types into a default text representation.
    /// Null and Callback are converted into an empty string.
    /// Lists and maps are converted into compact diagnostic placeholders.
    [[nodiscard]] auto toString() const -> String;

private:
    /// Convert a checked integral constructor argument.
    template <std::integral T>
    [[nodiscard]] static auto checkedInteger(const T value) -> int64_t {
        if (!std::in_range<int64_t>(value)) {
            throw err::ParameterError{
                StringLiteral{"The render integer is outside the signed 64-bit range."}, StringLiteral{"value"}};
        }
        return static_cast<int64_t>(value);
    }

private:
    Storage _storage;
};

}
