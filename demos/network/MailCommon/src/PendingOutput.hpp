// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringEncoding.hpp>

namespace demo {

using namespace el::text::literals;

/// Retain bounded application output until the connection accepts each complete block.
class PendingOutput {
public: // output queue
    /// Queue ASCII protocol text, with room for a bounded message and SMTP framing.
    /// @param text Text whose ASCII validation belongs to the protocol layer.
    /// @throws el::ApplicationError If queued output exceeds 128 KiB.
    void append(const el::String &text) {
        const auto bytes = el::StringEncoder{text}.encode(el::StringEncoding::Utf8);
        if (_bytes.length() + bytes.length() > el::ByteCount{128 * 1024}) {
            throw el::ApplicationError{"SMTP output queue is full."_el};
        }
        _bytes.append(bytes);
    }
    /// Test whether all application output has been accepted.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _bytes.isEmpty(); }
    /// Get the next atomic TLS send, retaining ownership until acceptance.
    [[nodiscard]] auto nextBlock() const -> el::ByteBlock {
        return _bytes.slice(el::ByteIndex::zero(), el::ByteCount{16 * 1024});
    }
    /// Remove exactly the block accepted by the connection.
    /// @param length The length of the last accepted block.
    void accepted(const el::ByteCount length) { _bytes.remove(el::ByteRange{el::ByteIndex::zero(), length}); }

private:
    el::ByteBlockEditor _bytes; ///< Output not yet accepted by TLS.
};

}
