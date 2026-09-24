// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompressionError.hpp"

#include "impl/CompressionErrorDiagnostic.hpp"

#include "../text/String.hpp"

#include <memory>
#include <utility>

namespace erbsland::compression {

CompressionError::CompressionError(const CompressionErrorReason reason, text::String message) noexcept :
    CompressionError{CompressionErrorContext{reason, std::move(message)}} {
}

CompressionError::CompressionError(const CompressionErrorReason reason, const std::string_view message) noexcept :
    CompressionError{CompressionErrorContext{reason, text::String{message}}} {
}

CompressionError::CompressionError(CompressionErrorContext context) noexcept :
    err::RuntimeError{context.title()}, _context{std::move(context)} {
}

auto CompressionError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::CompressionErrorDiagnostic>(_context);
}

}
