// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::system::impl {

class SystemInfoBackend;
using SystemInfoBackendPtr = std::unique_ptr<SystemInfoBackend>;

}
