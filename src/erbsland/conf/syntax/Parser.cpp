// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Parser.hpp"

#include "impl/Reader.hpp"

namespace erbsland::conf::syntax {

auto Parser::parseText(text::String text, SourceIdentifierPtr identifier) const -> DocumentPtr {
    return impl::Reader{}.read(std::move(text), std::move(identifier));
}

}
