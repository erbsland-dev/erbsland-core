// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::system {

class PlatformErrorContext;
using PlatformErrorContextPtr = std::shared_ptr<PlatformErrorContext>;
using PlatformErrorContextConstPtr = std::shared_ptr<const PlatformErrorContext>;

}
