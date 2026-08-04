// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../String.hpp"

namespace erbsland::text::punycode::impl {

/// Encode one Unicode sequence with the overflow-checked RFC 3492 algorithm.
/// @tested{PunycodeTest}
[[nodiscard]] auto encodePunycodePayload(const String &text) -> String;
/// Decode one pure ASCII payload with the overflow-checked RFC 3492 algorithm.
/// @tested{PunycodeTest}
[[nodiscard]] auto decodePunycodePayload(const String &text) -> String;

}
