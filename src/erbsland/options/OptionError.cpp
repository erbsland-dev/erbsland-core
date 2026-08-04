// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionError.hpp"

#include "impl/OptionErrorDiagnostic.hpp"

#include <memory>

namespace erbsland::options {

OptionError::OptionError(text::String reason, const std::exception_ptr &cause) noexcept :
    OptionError(OptionErrorContext{}.setDescription(std::move(reason)), cause) {
}

OptionError::OptionError(OptionErrorContext context) noexcept : OptionError(std::move(context), {}) {
}

OptionError::OptionError(OptionErrorContext context, const std::exception_ptr &cause) noexcept :
    err::RuntimeError{context.title().isEmpty() ? context.description() : context.title(), cause},
    _context{std::move(context)} {
}

auto OptionError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::OptionErrorDiagnostic>(_context);
}
}
