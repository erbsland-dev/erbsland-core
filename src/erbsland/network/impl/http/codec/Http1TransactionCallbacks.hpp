// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1DecodeEvent.hpp"
#include "Http1TransactionFailure.hpp"

#include "../../../source/NetworkDataFn.hpp"
#include "../../../source/NetworkEventFn.hpp"

#include <functional>

namespace erbsland::network::impl {

/// Owner-loop callbacks for one internal HTTP/1 transaction.
/// @tested{Http1TransactionTest}
struct Http1TransactionCallbacks final {
    std::function<void(const Http1DecodeEvent &)> requestHead;       ///< Server request-head checkpoint.
    std::function<void(const Http1DecodeEvent &)> responseHead;      ///< Client final response-head checkpoint.
    std::function<void(const Http1DecodeEvent &)> informationalHead; ///< Client informational response event.
    NetworkDataFn bodyData;                                          ///< One streamed decoded body block.
    NetworkDataFn aggregatedBody;                                    ///< One completed aggregated body.
    std::function<void(const HttpHeaders &)> trailers;               ///< Separate incoming trailer fields.
    NetworkEventFn inputComplete;                                    ///< Final incoming message completion.
    NetworkEventFn writable;                                         ///< Renewed semantic output capacity.
    std::function<void(const Http1TransactionFailure &)> failure;    ///< One categorized operational failure.
    std::function<void(bool)> complete;                              ///< Success and connection-reuse eligibility.
    NetworkEventFn final;                                            ///< Exactly-once transaction finalization.
};

}
