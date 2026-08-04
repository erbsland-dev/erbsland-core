// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::system::impl {

class EnvironmentVariableBackend;
using EnvironmentVariableBackendPtr = std::unique_ptr<EnvironmentVariableBackend>;

}
