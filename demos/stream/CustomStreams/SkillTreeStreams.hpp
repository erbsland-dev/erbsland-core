// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace demo {

/// A bounded byte-input stream for serialized character skill trees.
class SkillTreeInputStream final : public el::ByteInputStream {
public:
    [[nodiscard]] static auto create(std::vector<uint8_t> bytes) -> std::shared_ptr<SkillTreeInputStream>;
    explicit SkillTreeInputStream(std::vector<uint8_t> bytes);

public: // implement InputStream
    [[nodiscard]] auto inputSettings() const noexcept -> const el::InputStreamSettings & override { return _settings; }
    [[nodiscard]] auto state() const noexcept -> el::StreamState override { return _state; }
    [[nodiscard]] auto isReady() const noexcept -> bool override { return _state == el::StreamState::Open; }
    [[nodiscard]] auto waitForReady() -> el::StreamWaitStatus override;
    auto close() -> el::StreamCloseStatus override;
    void abort() noexcept override;

protected: // implement ByteInputStream
    [[nodiscard]] auto readFromSource(std::span<el::Byte> destination, ReadDeadline deadline)
        -> el::StreamReadResult<el::ByteLength> override;

private:
    std::vector<uint8_t> _bytes;
    std::size_t _position{0U};
    el::InputStreamSettings _settings;
    el::StreamState _state{el::StreamState::Open};
};

/// An atomic in-memory byte-output stream for serialized skill trees.
class SkillTreeOutputStream final : public el::ByteOutputStream {
public:
    [[nodiscard]] static auto create() -> std::shared_ptr<SkillTreeOutputStream>;

public: // accessors
    [[nodiscard]] auto bytes() const noexcept -> const std::vector<uint8_t> & { return _bytes; }

public: // implement OutputStream
    using el::ByteOutputStream::write;

    [[nodiscard]] auto outputSettings() const noexcept -> const el::OutputStreamSettings & override {
        return _settings;
    }
    [[nodiscard]] auto state() const noexcept -> el::StreamState override { return _state; }
    [[nodiscard]] auto isReady() const noexcept -> bool override { return _state == el::StreamState::Open; }
    [[nodiscard]] auto waitForReady() -> el::StreamWaitStatus override;
    auto flush() -> el::StreamWriteStatus override;
    auto close() -> el::StreamCloseStatus override;
    void abort() noexcept override;
    auto write(std::span<const el::Byte> bytes) -> el::StreamWriteStatus override;

private:
    std::vector<uint8_t> _bytes;
    el::OutputStreamSettings _settings;
    el::StreamState _state{el::StreamState::Open};
};

}
