// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

#include <atomic>
#include <cstddef>
#include <span>
#include <vector>

namespace demo {

/// A deterministic byte input stream used to demonstrate bounded operations.
class ScriptedByteInputStream final : public el::ByteInputStream {
public:
    explicit ScriptedByteInputStream(
        std::vector<uint8_t> bytes,
        std::size_t maximumChunk = 1024U,
        std::size_t timeoutCount = 0U,
        bool ready = true,
        bool blocked = false);

public: // controls
    void setFailure(bool value) noexcept { _failure = value; }
    void release() noexcept { _released = true; }
    [[nodiscard]] auto readCount() const noexcept -> std::size_t { return _readCount; }

public: // implement InputStream
    [[nodiscard]] auto inputSettings() const noexcept -> const el::InputStreamSettings & override { return _settings; }
    [[nodiscard]] auto state() const noexcept -> el::StreamState override { return _state; }
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> el::StreamWaitStatus override;
    auto close() -> el::StreamCloseStatus override;
    void abort() noexcept override;

protected: // implement ByteInputStream
    [[nodiscard]] auto readFromSource(std::span<el::Byte> destination, ReadDeadline deadline)
        -> el::StreamReadResult<el::ByteLength> override;

private:
    std::vector<uint8_t> _bytes;
    std::size_t _maximumChunk;
    std::size_t _timeoutCount;
    std::size_t _position{0U};
    std::atomic<std::size_t> _readCount{0U};
    bool _ready;
    bool _failure{false};
    bool _blocked;
    std::atomic<bool> _released{false};
    el::InputStreamSettings _settings;
    std::atomic<el::StreamState> _state{el::StreamState::Open};
};

}
