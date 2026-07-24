// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::util {

/// A type safe, extendable result type.
/// - Using `Result` instead of `bool` makes user code more readable and easier to understand.
/// - Resolves to one byte, same size as `bool`.
/// - May be extended to transport more states and more information.
/// - Supports multiple success and failure states.
/// Internally uses a single byte to store the result value.
/// Success values are in the range [0, 127], failure values are in the range [128, 255], counting down from 255.
/// The first success value is therefore 0, and the last success value is 127.
/// Read the documentation for more information how to write custom result types.
class Result {
protected:
    /// The internal numeric type for all result values.
    class Value {
        constexpr explicit Value(const uint8_t value) : value{value} {}

    public:
        /// Create a success value.
        template <uint8_t N>
        constexpr static auto success() noexcept -> Value {
            static_assert(N < uint8_t{0x80U});
            return Value{N};
        }
        /// Create a failure value.
        template <uint8_t N>
        constexpr static auto failure() noexcept -> Value {
            static_assert(N < uint8_t{0x80U});
            return Value{static_cast<std::uint8_t>(0xFFU - N)};
        }

    public:
        /// Raw byte representation of the result state.
        uint8_t value{};
    };

public:
    /// Create a new result.
    /// @param value The value of the result.
    constexpr Result(const Value value) : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~Result() = default;
    Result(const Result &) = default;
    auto operator=(const Result &) -> Result & = default;

public: // operators
    auto operator==(const Result &other) const -> bool { return _value.value == other._value.value; }
    auto operator!=(const Result &other) const -> bool { return _value.value != other._value.value; }

public: // tests
    /// Test if the call was successful.
    [[nodiscard]] constexpr auto isSuccessful() const -> bool { return _value.value < uint8_t{0x80U}; }
    /// Test if the call has failed.
    [[nodiscard]] constexpr auto isFailure() const -> bool { return _value.value >= uint8_t{0x80U}; }

    /// Test if the call was successful.
    friend constexpr auto isSuccessful(const Result &result) -> bool { return result.isSuccessful(); }
    /// Test if the call has failed.
    friend constexpr auto isFailure(const Result &result) -> bool { return result.isFailure(); }

public: // predefined values
    /// The canonical successful result.
    static const Result Success;
    /// The canonical failed result.
    static const Result Failure;

protected:
    /// Encoded result state.
    Value _value;
};

inline constexpr Result Result::Success = Result::Value::success<0>();
inline constexpr Result Result::Failure = Result::Value::failure<0>();

}
