// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../time/TimeDelta.hpp"
#include "../unit/ByteLength.hpp"

namespace erbsland::stream {

/// Settings fixed when an input stream is created.
/// @tested{StreamSettingsTest}
class InputStreamSettings final {
public:
    /// The default capacity of each input ring.
    static constexpr auto cDefaultBufferCapacity = unit::ByteLength{64U * 1024U};

public: // accessors
    /// Get the maximum wait for one public operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _timeout; }
    /// Set the maximum wait for one public operation.
    auto setTimeout(time::TimeDelta value) noexcept -> InputStreamSettings & {
        _timeout = value;
        return *this;
    }
    /// Get the capacity of each input ring.
    [[nodiscard]] auto bufferCapacity() const noexcept -> unit::ByteLength { return _bufferCapacity; }
    /// Set the capacity of each input ring.
    auto setBufferCapacity(unit::ByteLength value) noexcept -> InputStreamSettings & {
        _bufferCapacity = value;
        return *this;
    }

private:
    time::TimeDelta _timeout{time::TimeDelta::milliseconds(1000)};
    unit::ByteLength _bufferCapacity{cDefaultBufferCapacity};
};

}
