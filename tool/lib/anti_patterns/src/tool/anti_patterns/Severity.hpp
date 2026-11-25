// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace tool::anti_patterns {

/// Priority of an anti-pattern finding in the report.
enum class Severity : uint8_t { High, Medium, Low };

}
