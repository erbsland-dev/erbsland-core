// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::re::impl {

class EngineData;
using EngineDataPtr = std::shared_ptr<EngineData>;
using ConstEngineDataPtr = std::shared_ptr<const EngineData>;

}
