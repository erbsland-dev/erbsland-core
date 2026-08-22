// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpStaticContent_fwd.hpp"

#include "../../stream/ByteInputStream_fwd.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::network {

/// One static HTTP response body with a known length.
/// Implementations may prepare content lazily. `open()` is called at most once and must return a non-null stream
/// positioned at byte zero. Implementations are invoked on server workers and must be thread-safe.
/// @seedoc{/reference/network/http_server}
/// @tested{HttpStaticContentTest}
class HttpStaticContent {
public:
    // defaults
    virtual ~HttpStaticContent() = default;

public:
    /// Get the exact finite logical content length.
    [[nodiscard]] virtual auto length() const noexcept -> unit::ByteLength = 0;
    /// Get the memory retained while this content is active.
    /// The conservative default assumes the complete logical content is retained.
    [[nodiscard]] virtual auto retainedMemoryLength() const noexcept -> unit::ByteLength;
    /// Open the one-shot content stream at byte zero.
    [[nodiscard]] virtual auto open() -> stream::ByteInputStreamPtr = 0;
};

}
