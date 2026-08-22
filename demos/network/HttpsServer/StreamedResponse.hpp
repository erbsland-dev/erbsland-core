// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/network/http_server/HttpServerRequest.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace demo {

/// Pump a generated HTTP response while respecting atomic send back-pressure.
/// @notest{Compiled and exercised as part of the HTTPS server demo.}
class StreamedResponse final : public std::enable_shared_from_this<StreamedResponse> {
public:
    /// Callback invoked with the response address after its retained request becomes final.
    using DoneFn = std::function<void(StreamedResponse *)>;

    /// Create a response pump for one retained request.
    /// @param request The request that receives the streamed response.
    /// @param done Callback used by the application to release this pump.
    StreamedResponse(el::HttpServerRequestPtr request, DoneFn done) :
        _request{std::move(request)}, _done{std::move(done)} {}

    /// Commit the response head and start pumping generated body blocks.
    void start();

private:
    void pump();

private:
    /// Number of generated text lines.
    static constexpr auto cLineCount = std::size_t{128U};

    el::HttpServerRequestPtr _request; ///< Request retained until its final callback.
    DoneFn _done;                      ///< Application cleanup callback.
    el::ByteBlock _pendingBlock;       ///< Exact block awaiting atomic acceptance.
    std::size_t _lineIndex{};          ///< Index of the next generated line.
    bool _finished{};                  ///< Whether no more body output may be submitted.
};

}
