// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/u8/impl/U8StringLiteralFactory.hpp"
#include "../../ConfError.hpp"

#include <utility>

namespace erbsland::conf::impl {

using namespace text::literals;

/// Throw an internal error.
/// This dedicated method exists as a convenient breakpoint for debugging.
/// @param message The error message.
[[noreturn]] inline void throwInternalError(text::String message) {
    throw ConfError{ConfErrorCategory::Internal, std::move(message)};
}

/// Require an expression to be true.
/// @param condition The condition that must be true.
inline void require(const bool condition) {
    if (!condition) {
        throwInternalError("Assertion failed"_el);
    }
}

/// Require an expression to be true.
/// @param condition The condition that must be true.
/// @param message The message in case the condition is false.
template <std::size_t N>
inline void require(const bool condition, const char (&message)[N]) {
    if (!condition) {
        throwInternalError(text::impl::createU8StringLiteral(message, N - 1));
    }
}

// A test that is performed at runtime.
// We perform these tests at places where we want to ensure correctness for safety reasons.
#define ERBSLAND_CORE_CONF_REQUIRE_SAFETY(condition, message) ::erbsland::conf::impl::require(condition, message)
// A test that is only performed in debug builds.
// We perform these tests just to get better debugging information, but the program would fail safely otherwise.
#if defined(_DEBUG) || !defined(NDEBUG)
#define ERBSLAND_CORE_CONF_REQUIRE_DEBUG(condition, message) ::erbsland::conf::impl::require(condition, message)
#else
#define ERBSLAND_CORE_CONF_REQUIRE_DEBUG(condition, message)
#endif

}
