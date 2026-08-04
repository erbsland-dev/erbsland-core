// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsAlertDescription.hpp"

#include "../../err/RuntimeError.hpp"

namespace erbsland::network::impl {

/// Categorized internal TLS 1.3 protocol failure with its RFC alert mapping.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html#section-6
/// @tested{TlsWireCodecTest TlsClientProtocolTest}
class TlsProtocolError final : public err::RuntimeError {
public:
    /// Create a protocol failure.
    /// @param alert The alert description required by RFC 8446.
    /// @param reason Stable local diagnostic text.
    TlsProtocolError(TlsAlertDescription alert, text::String reason) noexcept;

    // defaults
    ~TlsProtocolError() override = default;

public: // accessors
    /// Get the alert description required for this failure.
    [[nodiscard]] auto alert() const noexcept -> TlsAlertDescription { return _alert; }

private:
    TlsAlertDescription _alert; ///< RFC 8446 alert description.
};

}
