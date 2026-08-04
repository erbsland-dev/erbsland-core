// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::stream {

class InputStream;
/// Shared pointer for InputStream.
using InputStreamPtr = std::shared_ptr<InputStream>;

}
