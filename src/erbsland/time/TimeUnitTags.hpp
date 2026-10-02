// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::time {

/// Unit tag for time amounts measured in seconds and fractions/multiples of seconds.
/// @tested{TimeCoreTest}
struct SecondsUnitTag {};
/// Unit tag for calendar month amounts.
/// @tested{TimeCoreTest}
struct MonthsUnitTag {};
/// Unit tag for calendar year amounts.
/// @tested{TimeCoreTest}
struct YearsUnitTag {};

}
