// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network {

class HttpServerRequest;

/// A shared HTTP server request.
using HttpServerRequestPtr = std::shared_ptr<HttpServerRequest>;

}
