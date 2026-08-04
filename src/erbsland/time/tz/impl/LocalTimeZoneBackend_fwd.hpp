// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::time::tz::impl {

class LocalTimeZoneBackend;
using LocalTimeZoneBackendPtr = std::unique_ptr<LocalTimeZoneBackend>;

}
