// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConnectionCloseContext_fwd.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving an orderly TLS connection closure.
using TlsClientConnectionCloseFn = std::function<void(const TlsClientConnectionCloseContext &)>;

}
