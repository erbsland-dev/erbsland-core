// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZipError.hpp"

#include "../../err/impl/ExceptionDiagnostic.hpp"
#include "../../text/Literals.hpp"

#include <memory>
#include <utility>

namespace erbsland::compression::zip {

using namespace text::literals;

ZipError::ZipError(ZipErrorContext context, std::exception_ptr cause) noexcept :
    err::RuntimeError{context.title(), std::move(cause)}, _context{std::move(context)} {
}

auto ZipError::diagnostic() const -> err::DiagnosticConstPtr {
    auto result = std::make_shared<err::impl::ExceptionDiagnostic>(_context.title());
    if (!_context.description().isEmpty()) {
        result->appendField("Description"_el, _context.description());
    }
    if (!_context.sourcePath().isEmpty()) {
        result->appendField("Source"_el, _context.sourcePath().toString(), "path"_el);
    }
    if (!_context.destinationPath().isEmpty()) {
        result->appendField("Destination"_el, _context.destinationPath().toString(), "path"_el);
    }
    if (!_context.itemPath().isEmpty()) {
        result->appendField("Archive item"_el, _context.itemPath().toString(), "path"_el);
    }
    return result;
}

}
