// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PlatformError.hpp"

#include "impl/PlatformErrorDiagnostic.hpp"

#include <memory>
#include <utility>

namespace erbsland::system {

PlatformError::PlatformError(
    text::StringView reason, PlatformErrorContextConstPtr context, std::exception_ptr cause) noexcept :
    err::RuntimeError{std::move(reason), std::move(cause)}, _context{std::move(context)} {
}

auto PlatformError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::PlatformErrorDiagnostic>(reason(), _context);
}

}
