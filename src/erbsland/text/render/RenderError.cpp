// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RenderError.hpp"

#include "../../err/impl/ExceptionDiagnostic.hpp"

namespace erbsland::text::render {

RenderError::RenderError(RenderErrorContext context, const std::exception_ptr &cause) :
    RuntimeError(context.title(), cause), _context(std::move(context)) {
}

auto RenderError::diagnostic() const -> err::DiagnosticConstPtr {
    using namespace text::literals;
    auto result = std::make_shared<err::impl::ExceptionDiagnostic>(_context.title());
    result->setSourceName(_context.layout()).setSourcePath(_context.origin()).setLocation(_context.location());
    if (!_context.description().isEmpty()) {
        result->appendField("description"_el, _context.description());
    }
    if (!_context.frames().isEmpty()) {
        result->appendField("render frames"_el, _context.frames().join(" → "_el));
    }
    return result;
}

}
