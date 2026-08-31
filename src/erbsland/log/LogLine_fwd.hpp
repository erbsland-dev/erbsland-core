// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::log {

class LogLine;
/// Shared pointer to a mutable formatted log line.
using LogLinePtr = std::shared_ptr<LogLine>;
/// Shared pointer to an immutable formatted log line.
using LogLineConstPtr = std::shared_ptr<const LogLine>;

}
