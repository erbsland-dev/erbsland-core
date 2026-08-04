// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsRecordState.hpp"

#include "../../tls_record/TlsRecordDecryptor.hpp"
#include "../../tls_record/TlsRecordEncryptor.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl {

/// Test-only access to TLS record counters without performing impractically long record loops.
/// @tested{TlsRecordProtectionTest}
class TlsRecordTestAccess final {
public:
    /// Access an encryptor's state.
    explicit TlsRecordTestAccess(TlsRecordEncryptor &encryptor) noexcept : _state{encryptor._state.get()} {}
    /// Access a decryptor's state.
    explicit TlsRecordTestAccess(TlsRecordDecryptor &decryptor) noexcept : _state{decryptor._state.get()} {}

public:
    /// Set the next sequence and successful-record count for a boundary test.
    void setCounters(const uint64_t sequenceNumber, const uint64_t recordCount) noexcept {
        _state->_sequenceNumber = sequenceNumber;
        _state->_recordCount = recordCount;
    }
    /// Get the next sequence number.
    [[nodiscard]] auto sequenceNumber() const noexcept -> uint64_t { return _state->_sequenceNumber; }
    /// Get the successful-record count.
    [[nodiscard]] auto recordCount() const noexcept -> uint64_t { return _state->_recordCount; }

private:
    TlsRecordState *_state; ///< Borrowed state under test.
};

}
