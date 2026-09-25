// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::named_key {

/// The kind of one parsed named-key entry.
enum class EntryKind : uint8_t {
    Key,          ///< A key without a value.
    KeyWithValue, ///< A key with an explicit or compact value.
    Value,        ///< A positional value.
    End,          ///< The configured end of the entry list.
};

}
