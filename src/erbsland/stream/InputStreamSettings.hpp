// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamBuffering.hpp"

#include "../time/TimeDelta.hpp"

namespace erbsland::stream {

/// Settings fixed when an input stream is created.
/// @tested{StreamSettingsTest}
class InputStreamSettings final {
public: // accessors
    /// Get the maximum wait for one public operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _timeout; }
    /// Set the maximum wait for one public operation.
    auto setTimeout(time::TimeDelta value) noexcept -> InputStreamSettings & {
        _timeout = value;
        return *this;
    }
    /// Get the intended balance between memory use and throughput.
    [[nodiscard]] auto buffering() const noexcept -> StreamBuffering { return _buffering; }
    /// Set the intended balance between memory use and throughput.
    auto setBuffering(StreamBuffering value) noexcept -> InputStreamSettings & {
        _buffering = value;
        return *this;
    }
    /// Test if library-owned input buffers use secure erasure.
    [[nodiscard]] auto isSensitive() const noexcept -> bool { return _sensitive; }
    /// Enable or disable secure erasure for library-owned input buffers.
    auto setSensitive(bool value) noexcept -> InputStreamSettings & {
        _sensitive = value;
        return *this;
    }

private:
    time::TimeDelta _timeout{time::TimeDelta::milliseconds(1000)};
    StreamBuffering _buffering{StreamBuffering::Balanced};
    bool _sensitive{false};
};

}
