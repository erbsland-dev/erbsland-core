// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String.hpp"
#include "../../Name.hpp"

namespace erbsland::conf::impl::vrc {

using namespace text::literals;

const inline static auto cEmptyName = Name{};
const inline static auto cReservedAny = Name{NameType::Regular, "vr_any"_el, PrivateTag{}};
const inline static auto cReservedTemplate = Name{NameType::Regular, "vr_template"_el, PrivateTag{}};
const inline static auto cReservedName = Name{NameType::Regular, "vr_name"_el, PrivateTag{}};
const inline static auto cReservedEntry = Name{NameType::Regular, "vr_entry"_el, PrivateTag{}};
const inline static auto cReservedKey = Name{NameType::Regular, "vr_key"_el, PrivateTag{}};
const inline static auto cReservedDependency = Name{NameType::Regular, "vr_dependency"_el, PrivateTag{}};
constexpr inline static auto cReservedEscape = "vr_vr_"_el;
constexpr inline static auto cReservedPrefix = "vr_"_el;

const inline static auto cUseTemplate = Name{NameType::Regular, "use_template"_el, PrivateTag{}};
const inline static auto cType = Name{NameType::Regular, "type"_el, PrivateTag{}};
const inline static auto cCaseSensitive = Name{NameType::Regular, "case_sensitive"_el, PrivateTag{}};

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

const inline static auto keyName = Name{NameType::Regular, "name"_el, PrivateTag{}};
const inline static auto keyKey = Name{NameType::Regular, "key"_el, PrivateTag{}};

const inline static auto depMode = Name{NameType::Regular, "mode"_el, PrivateTag{}};
const inline static auto depSource = Name{NameType::Regular, "source"_el, PrivateTag{}};
const inline static auto depTarget = Name{NameType::Regular, "target"_el, PrivateTag{}};
const inline static auto depError = Name{NameType::Regular, "error"_el, PrivateTag{}};

}
