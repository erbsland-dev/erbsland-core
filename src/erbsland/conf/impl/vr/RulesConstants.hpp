// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String.hpp"
#include "../../Name.hpp"

namespace erbsland::conf::impl::vrc {

using namespace text::literals;

constexpr inline static auto cReservedEscape = "vr_vr_"_el;
constexpr inline static auto cReservedPrefix = "vr_"_el;

constexpr inline static auto ctChars = "chars"_el;
constexpr inline static auto ctContains = "contains"_el;
constexpr inline static auto ctDefault = "default"_el;
constexpr inline static auto ctDescription = "description"_el;
constexpr inline static auto ctEnds = "ends"_el;
constexpr inline static auto ctEquals = "equals"_el;
constexpr inline static auto ctError = "error"_el;
constexpr inline static auto ctIn = "in"_el;
constexpr inline static auto ctIsOptional = "is_optional"_el;
constexpr inline static auto ctIsSecret = "is_secret"_el;
constexpr inline static auto ctKey = "key"_el;
constexpr inline static auto ctMatches = "matches"_el;
constexpr inline static auto ctMaximum = "maximum"_el;
constexpr inline static auto ctMaximumVersion = "maximum_version"_el;
constexpr inline static auto ctMinimum = "minimum"_el;
constexpr inline static auto ctMinimumVersion = "minimum_version"_el;
constexpr inline static auto ctMultiple = "multiple"_el;
constexpr inline static auto ctStarts = "starts"_el;
constexpr inline static auto ctTitle = "title"_el;
constexpr inline static auto ctVersion = "version"_el;

constexpr inline static auto ctPrefixNot = "not_"_el;
constexpr inline static auto ctSuffixError = "_error"_el;

}
