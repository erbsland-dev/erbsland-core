// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UdpSocketOptions_fwd.hpp"

#include "../../unit/ByteLength.hpp"

namespace erbsland::network {

/// Options captured when a UDP socket is started.
/// @tested{UdpSocketTest}
class UdpSocketOptions final {
public:
    /// The largest portable UDP payload size.
    inline static constexpr auto cMaximumPortableDatagramSize = unit::ByteLength{65'507U};
    /// The default maximum accepted datagram size.
    inline static constexpr auto cDefaultMaximumDatagramSize = cMaximumPortableDatagramSize;
    /// The default queued output limit.
    inline static constexpr auto cDefaultSendQueueLimit = unit::ByteLength{1024U * 1024U};

public: // accessors
    /// Get the largest accepted incoming or outgoing datagram payload.
    [[nodiscard]] constexpr auto maximumDatagramSize() const noexcept -> unit::ByteLength {
        return _maximumDatagramSize;
    }
    /// Set the largest accepted incoming or outgoing datagram payload.
    auto setMaximumDatagramSize(const unit::ByteLength value) noexcept -> UdpSocketOptions & {
        _maximumDatagramSize = value;
        return *this;
    }
    /// Get the maximum amount of queued outgoing payload data.
    [[nodiscard]] constexpr auto sendQueueLimit() const noexcept -> unit::ByteLength { return _sendQueueLimit; }
    /// Set the maximum amount of queued outgoing payload data.
    auto setSendQueueLimit(const unit::ByteLength value) noexcept -> UdpSocketOptions & {
        _sendQueueLimit = value;
        return *this;
    }

private:
    unit::ByteLength _maximumDatagramSize{cDefaultMaximumDatagramSize}; ///< Largest accepted datagram payload.
    unit::ByteLength _sendQueueLimit{cDefaultSendQueueLimit};           ///< Maximum queued outgoing payload data.
};

}
