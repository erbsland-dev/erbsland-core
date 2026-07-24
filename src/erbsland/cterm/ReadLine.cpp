// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadLine.hpp"

#include "impl/ReadLine.hpp"

#include <memory>
#include <utility>

namespace erbsland::cterm {

auto ReadLine::create(TerminalPtr terminal, ReadLineOptions options) -> ReadLinePtr {
    return std::make_shared<impl::ReadLine>(std::move(terminal), std::move(options));
}

}
