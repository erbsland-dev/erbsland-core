// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientBodyProgress.hpp"
#include "HttpClientRedirectAction.hpp"
#include "HttpClientRedirectContext.hpp"
#include "HttpClientRequest_fwd.hpp"
#include "HttpClientResponse_fwd.hpp"

#include "../http/HttpResponseHead.hpp"
#include "../source/NetworkErrorContext_fwd.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../text/json/JsonValue_fwd.hpp"
#include "../../text/String.hpp"

#include <functional>

namespace erbsland::network {

/// A completed bounded byte-response handler.
/// @tested{HttpClientTest}
using HttpClientResponseFn = std::function<void(HttpClientRequestPtr, HttpClientResponsePtr, mem::ByteBlock)>;
/// A completed bounded strict UTF-8 response handler.
/// @tested{HttpClientTest}
using HttpClientTextResponseFn = std::function<void(HttpClientRequestPtr, HttpClientResponsePtr, text::String)>;
/// A completed bounded JSON response handler.
/// @tested{HttpClientTest}
using HttpClientJsonResponseFn =
    std::function<void(HttpClientRequestPtr, HttpClientResponsePtr, text::json::JsonValue)>;
/// A low-level final-response-head handler.
/// @tested{HttpClientTest}
using HttpClientResponseHeadFn = std::function<void(HttpClientRequestPtr, HttpClientResponsePtr)>;
/// An informational response handler.
/// @tested{HttpClientTest}
using HttpClientInformationalResponseFn = std::function<void(HttpClientRequestPtr, const HttpResponseHead &)>;
/// A redirect-policy checkpoint handler.
/// @tested{HttpClientTest}
using HttpClientRedirectFn = std::function<HttpClientRedirectAction(HttpClientRequestPtr, HttpClientRedirectContext &)>;
/// A successfully committed output-sink progress handler.
/// @tested{HttpClientTest}
using HttpClientBodyProgressFn = std::function<void(const HttpClientBodyProgress &)>;
/// A request-associated operational-error handler.
/// @tested{HttpClientTest}
using HttpClientErrorFn = std::function<void(HttpClientRequestPtr, const NetworkErrorContext &)>;

}
