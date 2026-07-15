// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamError.hpp"

#include "impl/StreamErrorDiagnostic.hpp"

#include <memory>
#include <utility>

namespace erbsland::stream {

StreamError::StreamError(StreamErrorContext context, std::exception_ptr cause) noexcept :
    err::RuntimeError{context.title(), std::move(cause)}, _context{std::move(context)} {
}

auto StreamError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::StreamErrorDiagnostic>(_context);
}

}
