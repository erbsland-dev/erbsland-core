// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerRequest_fwd.hpp"
#include "HttpServerSession_fwd.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/json/JsonValue.hpp"
#include "../../text/String.hpp"

#include <functional>

namespace erbsland::network {

/// An aggregated byte-request handler.
/// @tested{HttpServerLiveTest HttpServerSessionTest}
using HttpServerRequestFn = std::function<void(HttpServerSessionPtr, HttpServerRequestPtr, mem::ByteBlock)>;

/// An aggregated strict UTF-8 request handler.
/// @tested{HttpServerLiveTest HttpServerSessionTest}
using HttpServerTextRequestFn = std::function<void(HttpServerSessionPtr, HttpServerRequestPtr, text::String)>;

/// An aggregated JSON request handler.
/// @tested{HttpServerLiveTest HttpServerSessionTest}
using HttpServerJsonRequestFn = std::function<void(HttpServerSessionPtr, HttpServerRequestPtr, text::json::JsonValue)>;

/// A low-level request-head handler selecting its body policy manually.
/// @tested{HttpServerLiveTest HttpServerSessionTest}
using HttpServerRequestHeadFn = std::function<void(HttpServerSessionPtr, HttpServerRequestPtr)>;

/// A new-session handler.
/// @tested{HttpServerLiveTest}
using HttpServerSessionFn = std::function<void(HttpServerSessionPtr)>;

}
