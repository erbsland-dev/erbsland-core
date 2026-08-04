// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientProtocol_fwd.hpp"

#include <functional>

namespace erbsland::network::impl {

/// Internal injection point for deterministic TLS client start inputs.
using TlsClientProtocolStartFn = std::function<void(TlsClientProtocol &)>;

}
