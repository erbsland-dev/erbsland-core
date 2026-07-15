// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionError.hpp"

#include "impl/OptionErrorDiagnostic.hpp"

#include <memory>

namespace erbsland::options {

OptionError::OptionError(OptionErrorContext context) :
    err::RuntimeError{context.title().isEmpty() ? context.description() : context.title()},
    _context{std::move(context)} {
}

auto OptionError::diagnostic() const -> err::DiagnosticConstPtr {
    return std::make_shared<impl::OptionErrorDiagnostic>(_context);
}

}
