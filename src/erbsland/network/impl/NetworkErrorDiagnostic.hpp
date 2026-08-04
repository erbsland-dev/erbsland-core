// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../source/NetworkErrorContext.hpp"

#include "../../err/Diagnostic.hpp"

namespace erbsland::network::impl {

/// Structured diagnostic for an asynchronous network error.
/// @tested{HostLookupTest}
class NetworkErrorDiagnostic final : public err::Diagnostic {
public:
    /// Create a diagnostic for a network error context.
    explicit NetworkErrorDiagnostic(NetworkErrorContext context) noexcept;

public: // implement Diagnostic
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument override;

private:
    NetworkErrorContext _context; ///< The context rendered by this diagnostic.
};

}
