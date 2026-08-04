// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/Result.hpp"

namespace erbsland::cterm {

/// The status of an interactive read-line operation.
/// Only `Committed` is a successful `util::Result`; all other states carry no caller-visible text.
/// @tested{ReadLineTest}
class ReadLineStatus : public util::Result {
public:
    using Result::Result;

public: // operators
    /// Compare two read-line status values.
    constexpr auto operator==(const ReadLineStatus &other) const noexcept -> bool {
        return _value.value == other._value.value;
    }

public: // tests
    /// Test if text was committed.
    [[nodiscard]] constexpr auto isCommitted() const noexcept -> bool { return *this == Committed; }
    /// Test if a non-blocking update found no terminal result.
    [[nodiscard]] constexpr auto isIdle() const noexcept -> bool { return *this == Idle; }
    /// Test if the user cancelled the operation.
    [[nodiscard]] constexpr auto isCancelled() const noexcept -> bool { return *this == Cancelled; }
    /// Test if the inactivity timeout expired.
    [[nodiscard]] constexpr auto isTimeout() const noexcept -> bool { return *this == Timeout; }

public:
    static const ReadLineStatus Committed; ///< The user committed the entered text.
    static const ReadLineStatus Idle;      ///< No terminal result is available yet.
    static const ReadLineStatus Cancelled; ///< The user cancelled the operation.
    static const ReadLineStatus Timeout;   ///< The inactivity timeout expired.
};

inline constexpr ReadLineStatus ReadLineStatus::Committed = Value::success<0>();
inline constexpr ReadLineStatus ReadLineStatus::Idle = Value::failure<0>();
inline constexpr ReadLineStatus ReadLineStatus::Cancelled = Value::failure<1>();
inline constexpr ReadLineStatus ReadLineStatus::Timeout = Value::failure<2>();

}
