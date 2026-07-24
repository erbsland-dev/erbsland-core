// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathBackend.hpp"

namespace erbsland::path::impl {

/// Access the platform-specific path backend.
[[nodiscard]] auto pathBackend() noexcept -> PathBackend &;

/// Create the default platform-specific path backend.
/// Implemented in `PosixPathBackend.cpp` and `WindowsPathBackend.cpp`.
[[nodiscard]] auto createPathBackend() noexcept -> PathBackendPtr;

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
/// Function for unit tests to replace the path backend.
/// - This call is not thread-safe.
/// - Can be called repeatedly.
/// - If called with a null pointer, the default backend is recreated.
void setPathBackend(PathBackendPtr &&backend) noexcept;
#endif

}
