// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathError.hpp"

#include "impl/PathErrorDiagnostic.hpp"

#include <memory>
#include <utility>

namespace erbsland::path {

PathError::PathError(text::String title, std::exception_ptr cause) noexcept :
    PathError{PathErrorContext{std::move(title)}, std::move(cause)} {
}

PathError::PathError(PathErrorContext context, std::exception_ptr cause) noexcept :
    err::RuntimeError{context.title(), std::move(cause)}, _context{std::move(context)} {
}

auto PathError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::PathErrorDiagnostic>(_context);
}

}
