// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/stream/ByteInputStream.hpp>
#include <erbsland/stream/InputStreamSettings.hpp>
#include <erbsland/stream/StreamCloseStatus.hpp>
#include <erbsland/stream/StreamPositionOrigin.hpp>
#include <erbsland/stream/StreamPositionStatus.hpp>
#include <erbsland/stream/StreamReadStatus.hpp>
#include <erbsland/stream/StreamState.hpp>
#include <erbsland/stream/StreamWaitStatus.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace erbsland::test {

/// Byte-block input stream that records reads, seeks, and ownership transitions for ZIP tests.
/// @notest{Test infrastructure used by ZipArchiveTest.}
class ZipTrackingInputStream final : public stream::ByteInputStream {
public:
    /// Create a tracking stream over `data`, optionally without positioning support.
    ZipTrackingInputStream(mem::ByteBlock data, const bool positionable) :
        _data{std::move(data)}, _positionable{positionable} {}

public: // implement InputStream
    [[nodiscard]] auto inputSettings() const noexcept -> const stream::InputStreamSettings & override {
        return _settings;
    }
    [[nodiscard]] auto state() const noexcept -> stream::StreamState override { return _state; }
    [[nodiscard]] auto isReady() const noexcept -> bool override { return true; }
    [[nodiscard]] auto waitForReady() -> stream::StreamWaitStatus override { return stream::StreamWaitStatus::Ready; }
    auto close() -> stream::StreamCloseStatus override {
        ++closeCount;
        _state = stream::StreamState::Closed;
        return stream::StreamCloseStatus::Closed;
    }
    void abort() noexcept override {
        ++abortCount;
        _state = stream::StreamState::Closed;
    }

protected: // implement ByteInputStream
    [[nodiscard]] auto readFromSource(mem::ByteSpan destination, ReadDeadline)
        -> stream::StreamReadResult<unit::ByteLength> override {
        const auto readOffset = _position;
        const auto available = _data.length().toSizeT() - _position;
        const auto count = std::min(destination.size(), available);
        for (auto index = std::size_t{}; index < count; ++index) {
            destination.data()[index] = _data.get(unit::ByteIndex::fromSizeT(_position + index));
        }
        _position += count;
        bytesRead += count;
        readCalls.emplace_back(readOffset, count);
        return {
            count == 0U ? stream::StreamReadStatus::Finished : stream::StreamReadStatus::Data,
            unit::ByteLength::fromSizeT(count)};
    }
    [[nodiscard]] auto sourceSupportsPositioning() const noexcept -> bool override { return _positionable; }
    [[nodiscard]] auto sourcePosition() const -> unit::ByteIndex override {
        return unit::ByteIndex::fromSizeT(_position);
    }
    auto setSourcePosition(const unit::ByteIndex position) -> stream::StreamPositionStatus override {
        _position = position.toSizeT();
        seekPositions.push_back(position.toRawValue());
        return stream::StreamPositionStatus::Success;
    }
    auto moveSourcePosition(const stream::StreamPositionOrigin origin, const unit::ByteOffset offset)
        -> stream::StreamPositionStatus override {
        const auto base = origin == stream::StreamPositionOrigin::Start
            ? unit::ByteIndex{}
            : (origin == stream::StreamPositionOrigin::Current ? unit::ByteIndex::fromSizeT(_position)
                                                               : _data.endIndex());
        _position = base.movedOrThrow(offset).toSizeT();
        return stream::StreamPositionStatus::Success;
    }

public:
    std::size_t bytesRead{};
    std::size_t closeCount{};
    std::size_t abortCount{};
    std::vector<uint64_t> seekPositions;
    std::vector<std::pair<uint64_t, std::size_t>> readCalls;

private:
    mem::ByteBlock _data;
    bool _positionable{};
    std::size_t _position{};
    stream::InputStreamSettings _settings;
    stream::StreamState _state{stream::StreamState::Open};
};

}
