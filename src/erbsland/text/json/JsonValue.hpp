// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "JsonFormatOptions.hpp"
#include "JsonParseOptions.hpp"
#include "JsonType.hpp"
#include "JsonValue_fwd.hpp"

#include "impl/JsonNativeType.hpp"
#include "impl/JsonValueData_fwd.hpp"

#include "../Literals.hpp"
#include "../String.hpp"
#include "../StringMap.hpp"

#include "../../err/ParameterError.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../unit/ItemIndex.hpp"
#include "../../util/List.hpp"

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace erbsland::text::json {

/// A copy-on-write JSON value tree.
/// @seedoc{/reference/text/documents_and_rendering}
/// @tested{JsonValueTest JsonParserTest}
class JsonValue final {
public:
    /// Create the null value.
    JsonValue() noexcept = default;
    /// Create a boolean value.
    JsonValue(bool value); // NOLINT(*-explicit-constructor)
    /// Create an integer number.
    JsonValue(int64_t value); // NOLINT(*-explicit-constructor)
    /// Create a floating-point number.
    /// @throws err::ParameterError If `value` is not finite.
    JsonValue(double value); // NOLINT(*-explicit-constructor)
    /// Create a text value.
    JsonValue(String value); // NOLINT(*-explicit-constructor)
    /// Create an array value.
    JsonValue(JsonArray value); // NOLINT(*-explicit-constructor)
    /// Create an object value.
    JsonValue(JsonObject value); // NOLINT(*-explicit-constructor)
    /// Create an integer number from another integral type.
    /// @throws err::ParameterError If `value` does not fit into `int64_t`.
    template <std::integral T>
        requires(!std::same_as<std::remove_cv_t<T>, bool> && !std::same_as<std::remove_cv_t<T>, int64_t>)
    JsonValue(const T value) : JsonValue{checkedInteger(value)} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~JsonValue() = default;
    JsonValue(const JsonValue &) noexcept = default;
    JsonValue(JsonValue &&) noexcept = default;
    auto operator=(const JsonValue &) noexcept -> JsonValue & = default;
    auto operator=(JsonValue &&) noexcept -> JsonValue & = default;

public: // tests/accessors
    /// Get the semantic JSON type.
    [[nodiscard]] auto type() const noexcept -> JsonType;
    /// Test whether this value has the requested type.
    [[nodiscard]] auto is(JsonType expected) const noexcept -> bool;
    /// Test whether this value is null, boolean, number, or text.
    [[nodiscard]] auto isPrimitive() const noexcept -> bool;
    /// Get the number of array elements or object entries, or zero for a primitive.
    [[nodiscard]] auto itemCount() const noexcept -> unit::ItemCount;

public: // child values
    /// Get an array element, or null if this is not an array or the index is invalid.
    [[nodiscard]] auto get(unit::ItemIndex index) const -> JsonValue;
    /// Get an array element.
    /// @throws err::LogicError If this is not an array.
    /// @throws err::OutOfRangeError If the index is invalid.
    [[nodiscard]] auto getOrThrow(unit::ItemIndex index) const -> JsonValue;
    /// Get an object member, or null if this is not an object or the key is missing.
    [[nodiscard]] auto get(const String &key) const -> JsonValue;
    /// Get an object member.
    /// @throws err::LogicError If this is not an object.
    /// @throws err::OutOfRangeError If the key is missing.
    [[nodiscard]] auto getOrThrow(const String &key) const -> JsonValue;

public: // typed values
    /// Get this value as a supported native type.
    template <typename T>
        requires impl::JsonNativeType<T>
    [[nodiscard]] auto get() const -> std::optional<T>;
    /// Get this value as a supported native type, or return a fallback.
    template <typename T>
        requires impl::JsonNativeType<T>
    [[nodiscard]] auto get(T fallback) const -> T;
    /// Get this value as a supported native type.
    /// @throws err::LogicError If the value cannot be represented as `T`.
    template <typename T>
        requires impl::JsonNativeType<T>
    [[nodiscard]] auto getOrThrow() const -> T;
    /// Get a boolean value.
    [[nodiscard]] auto getBool() const noexcept -> std::optional<bool>;
    /// Get a boolean value or a fallback.
    [[nodiscard]] auto getBool(bool fallback) const noexcept -> bool;
    /// Get a boolean value.
    /// @throws err::LogicError If this is not a boolean.
    [[nodiscard]] auto getBoolOrThrow() const -> bool;
    /// Get a number as a double.
    [[nodiscard]] auto getNumber() const noexcept -> std::optional<double>;
    /// Get a number as a double or a fallback.
    [[nodiscard]] auto getNumber(double fallback) const noexcept -> double;
    /// Get a number as a double.
    /// @throws err::LogicError If this is not a number.
    [[nodiscard]] auto getNumberOrThrow() const -> double;
    /// Get a text value.
    [[nodiscard]] auto getText() const noexcept -> std::optional<String>;
    /// Get a text value or a fallback.
    [[nodiscard]] auto getText(String fallback) const noexcept -> String;
    /// Get a text value.
    /// @throws err::LogicError If this is not text.
    [[nodiscard]] auto getTextOrThrow() const -> String;

public: // mutation
    /// Replace an array element, or append when `index == itemCount()`.
    /// @throws err::LogicError If this is not an array.
    /// @throws err::OutOfRangeError If `index > itemCount()`.
    auto set(unit::ItemIndex index, JsonValue value) -> JsonValue &;
    /// Insert or replace an object member.
    /// @throws err::LogicError If this is not an object.
    auto set(const String &key, JsonValue value) -> JsonValue &;
    /// Append an array element.
    /// @throws err::LogicError If this is not an array.
    auto append(JsonValue value) -> JsonValue &;

public: // conversion
    /// Serialize this value as valid JSON.
    [[nodiscard]] auto toString(JsonFormatOptions options = {}) const -> String;
    /// Parse one complete JSON value.
    /// @note UTF-8 errors are ignored, validate the string before parsing if this is relevant.
    [[nodiscard]] static auto fromString(const String &text, JsonParseOptions options = {}) noexcept
        -> std::optional<JsonValue>;
    /// Parse one complete JSON value.
    /// @note UTF-8 errors are ignored, validate the string before parsing if this is relevant.
    /// @throws err::ParseError If the document is invalid or exceeds configured limits.
    [[nodiscard]] static auto fromStringOrThrow(const String &text, JsonParseOptions options = {}) -> JsonValue;

private:
    /// Convert a checked integral constructor argument.
    template <std::integral T>
    [[nodiscard]] static auto checkedInteger(const T value) -> int64_t {
        using namespace literals;
        if (!std::in_range<int64_t>(value)) {
            throw err::ParameterError{"The JSON integer is outside the signed 64-bit range."_el, "value"_el};
        }
        return static_cast<int64_t>(value);
    }
    /// Get this number as an integer if it is exactly representable.
    [[nodiscard]] auto getInteger() const noexcept -> std::optional<int64_t>;
    /// Get a copy of this array value.
    [[nodiscard]] auto getArray() const noexcept -> std::optional<JsonArray>;
    /// Get a copy of this object value.
    [[nodiscard]] auto getObject() const noexcept -> std::optional<JsonObject>;
    /// Throw the common wrong-type diagnostic.
    [[noreturn]] static void throwTypeError();
    /// Detach shared storage before mutation.
    void detach();

private:
    std::shared_ptr<impl::JsonValueData> _data; ///< Shared value data; null represents JSON null.
};

}

#include "JsonValue_get.tpp"
