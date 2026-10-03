// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamedResponse.hpp"

#include <erbsland/HttpFieldType.hpp>
#include <erbsland/HttpHeaders.hpp>
#include <erbsland/HttpMediaType.hpp>
#include <erbsland/HttpResponseHead.hpp>
#include <erbsland/HttpServerRequestEventEditor.hpp>
#include <erbsland/HttpStatus.hpp>
#include <erbsland/HttpVersion.hpp>
#include <erbsland/StringBomMode.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringEncoding.hpp>
#include <erbsland/StringFormat.hpp>

namespace demo {

using namespace el::text::literals;

/// Start a chunked response and register callbacks that continue or release the response at lifecycle checkpoints.
void StreamedResponse::start() {
    const auto weakSelf = weak_from_this();
    _request->events()
        .onWritable([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->pump();
            }
        })
        .onFinal([weakSelf]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->_done(self.get());
            }
        });

    auto headers = el::HttpHeaders{};
    headers.setField(el::HttpFieldType::TransferEncoding, "chunked"_el);
    headers.setContentType(el::HttpMediaType::fromStringOrThrow("text/plain; charset=utf-8"_el));
    _request->startResponse(
        el::HttpResponseHead{el::HttpVersion::Http11, el::HttpStatus::Ok, "OK"_el, std::move(headers)});
    pump();
}

/// Keep one rejected block unchanged and retry it only after the request reports writable capacity.
void StreamedResponse::pump() {
    while (!_finished && _lineIndex < cLineCount) {
        if (_pendingBlock.isEmpty()) {
            const auto line =
                el::StringFormat{"generated line {:03d}: event-driven HTTPS output\n"_el}.build(_lineIndex + 1U);
            _pendingBlock = el::StringEncoder{line}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject);
        }
        const auto status = _request->sendBody(_pendingBlock);
        if (status.wouldBlock()) {
            return;
        }
        if (status.isClosed()) {
            _finished = true;
            return;
        }
        _pendingBlock = {};
        ++_lineIndex;
    }
    if (!_finished) {
        _finished = true;
        _request->finishBody();
    }
}

}
