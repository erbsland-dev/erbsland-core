// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all_path.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

namespace demo {

/// A completed file hash transferred by value to the receiver loop.
struct HashResult {
    el::Path path;     ///< File processed by the worker.
    el::String digest; ///< Base16 SHA-256 digest.
};

}
