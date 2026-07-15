// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

#include <atomic>
#include <cstddef>
#include <span>
#include <vector>

namespace demo {

/// A deterministic byte output stream used to demonstrate back pressure and lifecycle states.
class ScriptedByteOutputStream final : public el::ByteOutputStream {
public:
    explicit ScriptedByteOutputStream(
        std::size_t writeTimeoutCount = 0U, std::size_t flushTimeoutCount = 0U, std::size_t closeTimeoutCount = 0U);

public: // controls
    void setFailure(bool value) noexcept { _failure = value; }
    void enablePositioning(std::size_t timeoutCount = 0U) noexcept {
        _positioning = true;
        _positionTimeoutCount = timeoutCount;
    }
    [[nodiscard]] auto bytes() const noexcept -> const std::vector<uint8_t> & { return _bytes; }

public: // implement OutputStream
    using el::ByteOutputStream::write;

    [[nodiscard]] auto outputSettings() const noexcept -> const el::OutputStreamSettings & override {
        return _settings;
    }
    [[nodiscard]] auto state() const noexcept -> el::StreamState override { return _state; }
    [[nodiscard]] auto isReady() const noexcept -> bool override;
    [[nodiscard]] auto waitForReady() -> el::StreamWaitStatus override;
    auto flush() -> el::StreamWriteStatus override;
    auto close() -> el::StreamCloseStatus override;
    void abort() noexcept override;
    auto write(std::span<const el::Byte> bytes) -> el::StreamWriteStatus override;

public: // implement StreamPositioning
    [[nodiscard]] auto supportsPositioning() const noexcept -> bool override { return _positioning; }
    [[nodiscard]] auto position() const -> el::ByteIndex override;
    auto setPosition(el::ByteIndex position) -> el::StreamPositionStatus override;
    auto movePosition(el::StreamPositionOrigin origin, el::ByteOffset offset) -> el::StreamPositionStatus override;

private:
    std::size_t _writeTimeoutCount;
    std::size_t _flushTimeoutCount;
    std::size_t _closeTimeoutCount;
    bool _failure{false};
    bool _positioning{false};
    std::size_t _positionTimeoutCount{0U};
    el::ByteIndex _position;
    std::vector<uint8_t> _bytes;
    el::OutputStreamSettings _settings;
    std::atomic<el::StreamState> _state{el::StreamState::Open};
};

}
