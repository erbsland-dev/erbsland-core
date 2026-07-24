// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamBuffering.hpp"

#include "../time/TimeDelta.hpp"
#include "../unit/ByteLength.hpp"

#include <optional>

namespace erbsland::stream {

/// Settings fixed when an output stream is created.
/// @tested{StreamSettingsTest}
class OutputStreamSettings final {
public: // accessors
    /// Get the maximum wait for one public operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _timeout; }
    /// Set the maximum wait for one public operation.
    auto setTimeout(time::TimeDelta value) noexcept -> OutputStreamSettings & {
        _timeout = value;
        return *this;
    }
    /// Get the intended balance between memory use and throughput.
    [[nodiscard]] auto buffering() const noexcept -> StreamBuffering { return _buffering; }
    /// Set the intended balance between memory use and throughput.
    auto setBuffering(StreamBuffering value) noexcept -> OutputStreamSettings & {
        _buffering = value;
        return *this;
    }
    /// Get the effective hard back-ring limit.
    [[nodiscard]] auto backBufferLimit() const noexcept -> unit::ByteLength;
    /// Set the hard back-ring limit.
    auto setBackBufferLimit(unit::ByteLength value) noexcept -> OutputStreamSettings & {
        _backBufferLimit = value;
        return *this;
    }
    /// Clear the explicit hard back-ring limit and use the selected buffering preset.
    auto clearBackBufferLimit() noexcept -> OutputStreamSettings & {
        _backBufferLimit.reset();
        return *this;
    }

private:
    time::TimeDelta _timeout{time::TimeDelta::milliseconds(1000)};
    StreamBuffering _buffering{StreamBuffering::Balanced};
    std::optional<unit::ByteLength> _backBufferLimit;
};

}
