// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::log::impl {

class ActiveLogConfiguration;
/// Shared pointer to an immutable active log configuration.
using ActiveLogConfigurationConstPtr = std::shared_ptr<const ActiveLogConfiguration>;

}
