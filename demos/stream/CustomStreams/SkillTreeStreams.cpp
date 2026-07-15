// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "SkillTreeStreams.hpp"

#include <algorithm>

namespace demo {

auto SkillTreeInputStream::create(std::vector<uint8_t> bytes) -> std::shared_ptr<SkillTreeInputStream> {
    return std::make_shared<SkillTreeInputStream>(std::move(bytes));
}

SkillTreeInputStream::SkillTreeInputStream(std::vector<uint8_t> bytes) : _bytes{std::move(bytes)} {
}

auto SkillTreeInputStream::waitForReady() -> el::StreamWaitStatus {
    return isReady() ? el::StreamWaitStatus::Ready : el::StreamWaitStatus::Timeout;
}

auto SkillTreeInputStream::close() -> el::StreamCloseStatus {
    discardRetainedInput();
    _state = el::StreamState::Closed;
    return el::StreamCloseStatus::Closed;
}

void SkillTreeInputStream::abort() noexcept {
    discardRetainedInput();
    _state = el::StreamState::Closed;
}

auto SkillTreeInputStream::readFromSource(
    const std::span<el::Byte> destination, [[maybe_unused]] const ReadDeadline deadline)
    -> el::StreamReadResult<el::ByteLength> {
    if (_state != el::StreamState::Open) {
        throwError("The skill tree could not be read."_el, "The input stream is closed."_el);
    }
    if (_position == _bytes.size()) {
        return {el::StreamReadStatus::Finished, el::ByteLength::zero()};
    }
    const auto count = std::min({destination.size(), _bytes.size() - _position, std::size_t{3U}});
    for (auto index = std::size_t{0U}; index < count; ++index) {
        destination[index] = el::Byte{_bytes[_position + index]};
    }
    _position += count;
    return {el::StreamReadStatus::Data, el::ByteLength::fromSizeT(count)};
}

auto SkillTreeOutputStream::create() -> std::shared_ptr<SkillTreeOutputStream> {
    return std::make_shared<SkillTreeOutputStream>();
}

auto SkillTreeOutputStream::waitForReady() -> el::StreamWaitStatus {
    return isReady() ? el::StreamWaitStatus::Ready : el::StreamWaitStatus::Timeout;
}

auto SkillTreeOutputStream::flush() -> el::StreamWriteStatus {
    if (_state != el::StreamState::Open) {
        throwError("The skill tree could not be flushed."_el, "The output stream is not open."_el);
    }
    return el::StreamWriteStatus::Success;
}

auto SkillTreeOutputStream::close() -> el::StreamCloseStatus {
    _state = el::StreamState::Closed;
    return el::StreamCloseStatus::Closed;
}

void SkillTreeOutputStream::abort() noexcept {
    _bytes.clear();
    _state = el::StreamState::Closed;
}

auto SkillTreeOutputStream::write(const std::span<const el::Byte> bytes) -> el::StreamWriteStatus {
    if (_state != el::StreamState::Open) {
        throwError("The skill tree could not be written."_el, "The output stream is not open."_el);
    }
    auto accepted = std::vector<uint8_t>{};
    accepted.reserve(bytes.size());
    for (const auto byte : bytes) {
        accepted.push_back(byte.toUInt8());
    }
    _bytes.insert(_bytes.end(), accepted.begin(), accepted.end());
    return el::StreamWriteStatus::Success;
}

}
