// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::re::impl {

class CaptureGroupManager;
using CaptureGroupManagerPtr = std::unique_ptr<CaptureGroupManager>;

}
