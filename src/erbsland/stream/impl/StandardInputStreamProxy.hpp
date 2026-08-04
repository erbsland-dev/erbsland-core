// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StandardStreamSlot.hpp"

#include "../TextInputStream.hpp"

namespace erbsland::stream::impl {

/// Stable proxy for the process standard input stream.
/// @tested{StandardStreamsTest}
class StandardInputStreamProxy final : public TextInputStream {
public:
    /// Create the standard input proxy.
    StandardInputStreamProxy() = default;
    /// dtor, aborts any pending inputs.
    ~StandardInputStreamProxy() override { abort(); }

    // defaults/deletions
    StandardInputStreamProxy(const StandardInputStreamProxy &) = delete;
    auto operator=(const StandardInputStreamProxy &) -> StandardInputStreamProxy & = delete;
    StandardInputStreamProxy(StandardInputStreamProxy &&) = delete;
    auto operator=(StandardInputStreamProxy &&) -> StandardInputStreamProxy & = delete;

public: // implement TextInputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto inputSettings() const noexcept -> const InputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    [[nodiscard]] auto readChar() -> StreamReadResult<text::Char> override;
    [[nodiscard]] auto read(unit::CpLength maximum) -> StreamReadResult<text::String> override;
    [[nodiscard]] auto readLine(unit::CpLength maximum) -> StreamReadResult<text::String> override;
    [[nodiscard]] auto readAll(unit::CpLength maximum) -> StreamReadResult<text::String> override;

public:
    using TextInputStream::read;
    using TextInputStream::readAll;
    using TextInputStream::readLine;

private:
    /// Get the current proxied standard input stream.
    [[nodiscard]] auto target() const -> TextInputStreamPtr;
};

}
