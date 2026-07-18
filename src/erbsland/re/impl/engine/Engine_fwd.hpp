// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::re::impl {

class Engine;
using EnginePtr = std::shared_ptr<Engine>;
using ConstEnginePtr = std::shared_ptr<const Engine>;

}
