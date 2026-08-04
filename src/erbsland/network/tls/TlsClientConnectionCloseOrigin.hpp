// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network {

/// The side that first initiated an orderly TLS closure.
enum class TlsClientConnectionCloseOrigin : uint8_t {
    Local,  ///< The local application requested closure first.
    Remote, ///< The peer sent close_notify first.
};

}
