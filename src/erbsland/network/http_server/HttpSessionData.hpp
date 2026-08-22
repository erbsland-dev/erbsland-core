// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerSession_fwd.hpp"

namespace erbsland::network {

/// Application-defined data associated with an HTTP server session.
/// @tested{HttpServerSessionTest}
class HttpSessionData {
public:
    // defaults
    virtual ~HttpSessionData() = default;
};

}
