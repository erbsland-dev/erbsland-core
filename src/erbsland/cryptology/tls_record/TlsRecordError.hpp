// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsRecordErrorCategory.hpp"

#include "../../err/RuntimeError.hpp"

namespace erbsland::cryptology {

/// A categorized failure from TLS 1.3 record protection or deprotection.
/// Categories preserve the RFC 8446 alert distinction required by sections 5 and 6 without coupling cryptology to
/// network alert serialization.
/// @seedoc{/reference/cryptology/tls}
/// @tested{TlsRecordProtectionTest}
class TlsRecordError final : public err::RuntimeError {
public:
    /// Create a categorized record error.
    TlsRecordError(TlsRecordErrorCategory category, text::String reason) noexcept;

    // defaults
    ~TlsRecordError() override = default;

public: // accessors
    /// Get the stable failure category.
    [[nodiscard]] auto category() const noexcept -> TlsRecordErrorCategory { return _category; }

private:
    TlsRecordErrorCategory _category; ///< Stable record failure category.
};

}
