// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::log::impl {

class LogLineFormatter;
/// Shared pointer to a log line formatter.
using LogLineFormatterPtr = std::shared_ptr<LogLineFormatter>;
/// Shared pointer to an immutable log line formatter.
using LogLineFormatterConstPtr = std::shared_ptr<const LogLineFormatter>;

}
