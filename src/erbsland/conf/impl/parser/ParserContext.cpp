// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParserContext.hpp"

#include "../../../text/Literals.hpp"

#include <limits>

namespace erbsland::conf::impl {

using namespace text::literals;

ParserContext::ParserContext(
    const std::size_t includeLevel,
    SourcePtr source,
    text::placeholder::impl::RegistryPtr placeholderRegistry,
    PrivateTag /*pt*/) noexcept :
    _includeLevel{static_cast<uint8_t>(includeLevel)},
    _source{std::move(source)},
    _lexer{Lexer::create(CharStream::create(_source), std::move(placeholderRegistry))},
    _assignmentStream(AssignmentStream::create(_lexer)) {
    // Include depth is limited by design; keep it small and cheap to copy.
    assert(includeLevel <= static_cast<std::size_t>(std::numeric_limits<uint8_t>::max()));
}

auto ParserContext::create(
    const std::size_t includeLevel, SourcePtr source, text::placeholder::impl::RegistryPtr placeholderRegistry)
    -> ParserContextPtr {
    return std::make_shared<ParserContext>(
        includeLevel, std::move(source), std::move(placeholderRegistry), PrivateTag{});
}

void ParserContext::initialize() {
    if (_initialized) {
        throw err::LogicError("ParserContext::initialize() called twice."_el);
    }
    if (!_source->isOpen()) {
        _source->open();
    }
    _assignmentGenerator = _assignmentStream->assignments();
    _hasNextAssignment = true; // AssignmentStream guarantees a terminal EndOfDocument assignment.
    _initialized = true;
}

auto ParserContext::nextAssignment() -> Assignment {
    if (!_hasNextAssignment) {
        throw err::LogicError{"ParserContext::nextAssignment() called after the end of the assignment stream."};
    }
    auto assignment = _assignmentGenerator.next();
    if (!assignment.has_value()) {
        _hasNextAssignment = false;
        throw err::LogicError{"The assignment stream ended without an EndOfDocument assignment."};
    }
    _hasNextAssignment = assignment->type() != AssignmentType::EndOfDocument;
    return std::move(*assignment);
}

void ParserContext::close() {
    _hasNextAssignment = false;
    _assignmentGenerator = {};
    _lexer = {};
    if (_source->isOpen()) {
        _source->close();
    }
    _source = {};
}

}
