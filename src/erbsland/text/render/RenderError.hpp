// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RenderErrorContext.hpp"

#include "../../err/RuntimeError.hpp"

namespace erbsland::text::render {

/// The primary error type for the layout renderer.
/// @seedoc{/reference/text/documents_and_rendering}
/// @tested{RenderEnvironmentTest}
class RenderError final : public err::RuntimeError {
public:
    /// Create a new render error from the given context.
    /// @param context The context of the error.
    /// @param cause The optional cause of the error.
    explicit RenderError(RenderErrorContext context, const std::exception_ptr &cause = {});

public:
    /// Access the context of the error.
    [[nodiscard]] auto context() const noexcept -> const RenderErrorContext & { return _context; }

public: // implement Exception
    /// Create a standard diagnostic view of this render error.
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

private:
    RenderErrorContext _context;
};

}
