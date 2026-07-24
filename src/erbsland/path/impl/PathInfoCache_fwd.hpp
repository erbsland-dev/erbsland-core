// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::path::impl {

class PathInfoCache;

class PathInfoCacheTrust;
using PathInfoCacheTrustPtr = std::shared_ptr<PathInfoCacheTrust>;
using PathInfoCacheTrustWeakPtr = std::weak_ptr<PathInfoCacheTrust>;

}
