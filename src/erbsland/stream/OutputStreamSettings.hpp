// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../time/TimeDelta.hpp"
#include "../unit/ByteLength.hpp"

namespace erbsland::stream {

/// Settings fixed when an output stream is created.
/// @tested{StreamSettingsTest}
class OutputStreamSettings final {
public:
    /// The default capacity of the fixed front ring.
    static constexpr auto cDefaultBufferCapacity = unit::ByteLength{64U * 1024U};
    /// The default hard limit of the growing back ring.
    static constexpr auto cDefaultBackBufferLimit = unit::ByteLength{10'000'000U};

public: // accessors
    /// Get the maximum wait for one public operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _timeout; }
    /// Set the maximum wait for one public operation.
    auto setTimeout(time::TimeDelta value) noexcept -> OutputStreamSettings & {
        _timeout = value;
        return *this;
    }
    /// Get the fixed front-ring capacity.
    [[nodiscard]] auto bufferCapacity() const noexcept -> unit::ByteLength { return _bufferCapacity; }
    /// Set the fixed front-ring capacity.
    auto setBufferCapacity(unit::ByteLength value) noexcept -> OutputStreamSettings & {
        _bufferCapacity = value;
        return *this;
    }
    /// Get the hard back-ring limit.
    [[nodiscard]] auto backBufferLimit() const noexcept -> unit::ByteLength { return _backBufferLimit; }
    /// Set the hard back-ring limit.
    auto setBackBufferLimit(unit::ByteLength value) noexcept -> OutputStreamSettings & {
        _backBufferLimit = value;
        return *this;
    }

private:
    time::TimeDelta _timeout{time::TimeDelta::milliseconds(1000)};
    unit::ByteLength _bufferCapacity{cDefaultBufferCapacity};
    unit::ByteLength _backBufferLimit{cDefaultBackBufferLimit};
};

}
