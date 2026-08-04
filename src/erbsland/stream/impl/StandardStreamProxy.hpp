// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StandardStreamSlot.hpp"

#include "../TextOutputStream.hpp"

namespace erbsland::stream::impl {

/// Stable proxy for one process standard stream.
/// @tested{StandardStreamsTest}
class StandardStreamProxy final : public TextOutputStream {
public:
    /// Create a proxy for the given stream slot.
    explicit StandardStreamProxy(StandardStreamSlot slot);
    /// dtor, aborts any pending inputs.
    ~StandardStreamProxy() override { abort(); }

    // defaults/deletions
    StandardStreamProxy(const StandardStreamProxy &) = delete;
    auto operator=(const StandardStreamProxy &) -> StandardStreamProxy & = delete;
    StandardStreamProxy(StandardStreamProxy &&) = delete;
    auto operator=(StandardStreamProxy &&) -> StandardStreamProxy & = delete;

public: // implement TextOutputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto outputSettings() const noexcept -> const OutputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto flush() -> StreamWriteStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;
    auto write(text::Char character) -> StreamWriteStatus override;
    auto write(const text::String &text) -> StreamWriteStatus override;
    auto writeLine() -> StreamWriteStatus override;
    auto writeLine(const text::String &text) -> StreamWriteStatus override;

private:
    /// Get the current proxied standard output stream.
    [[nodiscard]] auto target() const -> TextOutputStreamPtr;

private:
    StandardStreamSlot _slot; ///< The proxied standard stream slot.
};

}
