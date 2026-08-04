// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::stream {

class OutputStream;
/// Shared pointer for OutputStream.
using OutputStreamPtr = std::shared_ptr<OutputStream>;

}
