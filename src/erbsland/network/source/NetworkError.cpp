// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NetworkError.hpp"

#include "../impl/NetworkErrorDiagnostic.hpp"

#include <memory>
#include <utility>

namespace erbsland::network {

NetworkError::NetworkError(NetworkErrorContext context, std::exception_ptr cause) noexcept :
    err::RuntimeError{context.title(), std::move(cause)}, _context{std::move(context)} {
}

auto NetworkError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::NetworkErrorDiagnostic>(_context);
}

}
