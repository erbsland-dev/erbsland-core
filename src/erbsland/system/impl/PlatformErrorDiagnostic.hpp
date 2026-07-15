// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../PlatformErrorContext_fwd.hpp"

#include "../../err/Diagnostic.hpp"
#include "../../text/StringView.hpp"

namespace erbsland::system::impl {

/// Diagnostic for a native platform error and its captured context.
/// @tested{DiagnosticTest}
class PlatformErrorDiagnostic final : public err::Diagnostic {
public:
    PlatformErrorDiagnostic(text::StringView title, PlatformErrorContextConstPtr context) noexcept;

public: // implement Diagnostic
    [[nodiscard]] auto toString() const noexcept -> text::StringView override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument override;

private:
    text::StringView _title;
    PlatformErrorContextConstPtr _context;
};

}
