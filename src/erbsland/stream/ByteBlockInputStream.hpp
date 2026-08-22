// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteBlockInputStream_fwd.hpp"
#include "ByteInputStream.hpp"

#include "../mem/ByteBlock.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ByteOffset.hpp"

#include <mutex>

namespace erbsland::stream {

/// An immediate positional byte input stream retaining one copy-on-write byte block.
/// @tested{ByteBlockInputStreamTest}
class ByteBlockInputStream final : public ByteInputStream {
public:
    /// Create an open stream retaining the supplied bytes.
    /// @param data The byte block retained as the stream source.
    /// @param settings The immutable input settings.
    explicit ByteBlockInputStream(mem::ByteBlock data, InputStreamSettings settings = {});

    // defaults/deletions
    ~ByteBlockInputStream() override = default;
    ByteBlockInputStream(const ByteBlockInputStream &) = delete;
    ByteBlockInputStream(ByteBlockInputStream &&) = delete;
    auto operator=(const ByteBlockInputStream &) -> ByteBlockInputStream & = delete;
    auto operator=(ByteBlockInputStream &&) -> ByteBlockInputStream & = delete;

public: // implement InputStream
    [[nodiscard]] auto inputSettings() const noexcept -> const InputStreamSettings & override;
    [[nodiscard]] auto state() const noexcept -> StreamState override;
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> StreamWaitStatus override;
    auto close() -> StreamCloseStatus override;
    void abort() noexcept override;

protected: // implement ByteInputStream
    [[nodiscard]] auto readFromSource(mem::ByteSpan destination, ReadDeadline deadline)
        -> StreamReadResult<unit::ByteLength> override;
    [[nodiscard]] auto sourceSupportsPositioning() const noexcept -> bool override;
    [[nodiscard]] auto sourcePosition() const -> unit::ByteIndex override;
    auto setSourcePosition(unit::ByteIndex position) -> StreamPositionStatus override;
    auto moveSourcePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> StreamPositionStatus override;

private:
    /// Require the stream to be open while holding `_mutex`.
    void verifyOpen() const;

private:
    mem::ByteBlock _data;                  ///< Retained source bytes.
    InputStreamSettings _settings;         ///< Immutable stream settings.
    mutable std::mutex _mutex;             ///< Serializes state and source position.
    unit::ByteIndex _position;             ///< Next source byte.
    StreamState _state{StreamState::Open}; ///< Immediate stream lifecycle.
};

}
