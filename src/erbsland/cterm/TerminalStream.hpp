// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStyle.hpp"
#include "Terminal_fwd.hpp"
#include "TerminalStream_fwd.hpp"
#include "TerminalStreamSynchronization.hpp"

#include "impl/TerminalStreamData_fwd.hpp"

#include "../stream/TextOutputStream.hpp"

#include <memory>
#include <utility>

namespace erbsland::cterm {

/// A text output stream that writes to a terminal with a fixed base style.
/// @tested{TerminalStreamTest}
class TerminalStream final : public stream::TextOutputStream {
public:
    /// Create a terminal stream.
    /// @param terminal The terminal to write to.
    /// @param style The style applied to every write.
    /// @param synchronization Shared synchronization state for streams using the same terminal.
    /// @param settings The fixed timeout and buffer settings.
    explicit TerminalStream(
        TerminalPtr terminal,
        BlockStyle style = BlockStyle::reset(),
        TerminalStreamSynchronizationPtr synchronization = {},
        stream::OutputStreamSettings settings = {});

    ~TerminalStream() override { abort(); }

    // defaults/deletions
    TerminalStream(const TerminalStream &) = delete;
    TerminalStream(TerminalStream &&) = delete;
    auto operator=(const TerminalStream &) -> TerminalStream & = delete;
    auto operator=(TerminalStream &&) -> TerminalStream & = delete;

public:
    /// Create shared synchronization state.
    [[nodiscard]] static auto createSynchronization() -> TerminalStreamSynchronizationPtr;
    /// Create a shared terminal stream.
    [[nodiscard]] static auto create(
        TerminalPtr terminal,
        BlockStyle style = BlockStyle::reset(),
        TerminalStreamSynchronizationPtr synchronization = {},
        stream::OutputStreamSettings settings = {}) -> TerminalStreamPtr;
    /// Create synchronized output and error streams for a terminal.
    [[nodiscard]] static auto createStandardStreams(TerminalPtr terminal)
        -> std::pair<TerminalStreamPtr, TerminalStreamPtr>;

public: // implement TextOutputStream
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto effectiveEncoding() const noexcept -> text::StringEncoding override;
    [[nodiscard]] auto outputSettings() const noexcept -> const stream::OutputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> stream::StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> stream::StreamWaitStatus override;
    auto flush() -> stream::StreamWriteStatus override;
    auto close() -> stream::StreamCloseStatus override;
    void abort() noexcept override;
    auto write(text::Char character) -> stream::StreamWriteStatus override;
    auto write(const text::String &text) -> stream::StreamWriteStatus override;
    auto writeLine() -> stream::StreamWriteStatus override;
    auto writeLine(const text::String &text) -> stream::StreamWriteStatus override;

public: // accessors
    /// Get the terminal used by this stream.
    [[nodiscard]] auto terminal() const noexcept -> const TerminalPtr & { return _terminal; }
    /// Get the style applied to every write.
    [[nodiscard]] auto style() const -> BlockStyle;
    /// Set the style applied to every write.
    void setStyle(BlockStyle style);

private:
    TerminalPtr _terminal;                           ///< The terminal to write to.
    std::shared_ptr<impl::TerminalStreamData> _data; ///< Shared state retained by pending work.
};

}
