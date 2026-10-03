// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../PlatformErrorContext.hpp"
#include "../../SubprocessExitStatus.hpp"

namespace erbsland::system::impl {

/// Native abnormal process termination, independent of errno/GetLastError.
/// @tested{SubprocessEventTest}
class SubprocessErrorContext final : public PlatformErrorContext {
public:
    /// Capture the status of an abnormally terminated child.
    /// @param status The platform exit status.
    explicit SubprocessErrorContext(SubprocessExitStatus status);
    /// Capture an operational failure without a native exit status.
    /// @param description The failed operation.
    explicit SubprocessErrorContext(text::String description);

public: // implement PlatformErrorContext
    auto category() const noexcept -> PlatformErrorCategory override;
    auto toString() const noexcept -> text::String override;
    auto toTextDocument() const -> text::TextDocument override;

private:
    text::String _description;                                      ///< Optional operation failure description.
    SubprocessExitStatus _status{SubprocessExitStatus::exited(-1)}; ///< Native abnormal termination details.
};

}
