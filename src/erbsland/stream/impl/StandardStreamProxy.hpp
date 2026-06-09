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

    // defaults
    ~StandardStreamProxy() override = default;
    StandardStreamProxy(const StandardStreamProxy &) = delete;
    auto operator=(const StandardStreamProxy &) -> StandardStreamProxy & = delete;
    StandardStreamProxy(StandardStreamProxy &&) = delete;
    auto operator=(StandardStreamProxy &&) -> StandardStreamProxy & = delete;

public: // implement TextOutputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto isOpen() const noexcept -> bool override;
    void flush() override;
    void close() override;
    void write(text::Char character) override;
    void write(const text::StringView &text) override;
    void writeLine() override;
    void writeLine(const text::StringView &text) override;

private:
    [[nodiscard]] auto target() const -> TextOutputStreamPtr;

private:
    StandardStreamSlot _slot; ///< The proxied standard stream slot.
};

}
