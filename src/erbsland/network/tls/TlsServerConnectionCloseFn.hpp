// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerConnectionCloseContext_fwd.hpp"

#include <functional>

namespace erbsland::network {
/// Callback for orderly accepted TLS closure.
using TlsServerConnectionCloseFn = std::function<void(const TlsServerConnectionCloseContext &)>;
}
