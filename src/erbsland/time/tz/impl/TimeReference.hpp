// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::time::tz::impl {

/// Reference used for a transition lookup.
enum class TimeReference {
    Utc,
    Local,
};

}
