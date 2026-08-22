// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::core {

class ApplicationPartManagerAccess;
using ApplicationPartManagerAccessPtr = std::shared_ptr<ApplicationPartManagerAccess>;
using ApplicationPartManagerAccessWeakPtr = std::weak_ptr<ApplicationPartManagerAccess>;

class ApplicationPartManager;
using ApplicationPartManagerPtr = std::shared_ptr<ApplicationPartManager>;

}
