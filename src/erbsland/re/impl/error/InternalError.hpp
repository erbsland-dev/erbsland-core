// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/Literals.hpp"
#include "../../../text/String.hpp"
#include "../../RegExError.hpp"

#include <utility>

namespace erbsland::re::impl {

using namespace text::literals;

/// Throw an internal error.
/// This dedicated method exists as a convenient breakpoint for debugging.
/// @param message The error message.
[[noreturn]] inline void throwInternalError(text::String message) {
    throw RegExError{ErrorCategory::Internal, "Internal regular-expression failure"_el, std::move(message)};
}

// A test that is performed at runtime.
// We perform these tests at places where we want to ensure correctness for safety reasons.
#define ERBSLAND_CORE_RE_REQUIRE_SAFETY(condition, message)                                                            \
    do {                                                                                                               \
        if (!(condition)) [[unlikely]] {                                                                               \
            ::erbsland::re::impl::throwInternalError(message);                                                         \
        }                                                                                                              \
    } while (false)
// A test that is only performed in debug and unit-test builds.
// We perform these tests just to get better debugging information, but the program would fail safely otherwise.
#if defined(_DEBUG) || !defined(NDEBUG) || defined(ERBSLAND_UNITTEST_BUILD)
#define ERBSLAND_CORE_RE_REQUIRE_DEBUG(condition, message)                                                             \
    do {                                                                                                               \
        if (!(condition)) [[unlikely]] {                                                                               \
            ::erbsland::re::impl::throwInternalError(message);                                                         \
        }                                                                                                              \
    } while (false)
#else
#define ERBSLAND_CORE_RE_REQUIRE_DEBUG(condition, message)                                                             \
    do {                                                                                                               \
    } while (false)
#endif

}
