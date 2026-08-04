// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network {

class HostLookup;

/// A shared host lookup.
using HostLookupPtr = std::shared_ptr<HostLookup>;

}
