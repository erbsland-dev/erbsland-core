// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// The side that first initiated an orderly accepted TLS closure.
enum class TlsServerConnectionCloseOrigin : uint8_t {
    Local,  ///< The local application requested closure first.
    Remote, ///< The TLS client sent close_notify first.
};

}
