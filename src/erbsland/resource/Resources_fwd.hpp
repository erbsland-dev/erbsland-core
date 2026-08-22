// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::resource {

class Resources;
using ResourcesPtr = std::shared_ptr<Resources>;
using ResourcesConstPtr = std::shared_ptr<const Resources>;

}
