// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConnectionCloseOrigin_fwd.hpp"

namespace erbsland::network {

/// The side that first initiated an orderly connection closure.
enum class ConnectionCloseOrigin : std::uint8_t {
    Local,  ///< The local application requested closure first.
    Remote, ///< The remote peer completed its protocol-specific orderly close first.
};

}
