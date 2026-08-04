// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class SourceResolver;
/// Shared pointer for SourceResolver.
using SourceResolverPtr = std::shared_ptr<SourceResolver>;

}
