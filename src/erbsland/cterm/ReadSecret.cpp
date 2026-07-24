// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadSecret.hpp"

#include "impl/ReadSecret.hpp"

#include <memory>
#include <utility>

namespace erbsland::cterm {

auto ReadSecret::create(TerminalPtr terminal, ReadLineOptions options) -> ReadSecretPtr {
    return std::make_shared<impl::ReadSecret>(std::move(terminal), std::move(options));
}

}
