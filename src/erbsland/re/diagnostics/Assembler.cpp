// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Assembler.hpp"

#include "../impl/diagnostics/Assembler.hpp"

namespace erbsland::re::diagnostics {

auto Assembler::compile(const text::StringViewList &lines) const -> RegExPtr {
    impl::Assembler assembler{};
    auto engineData = assembler.compile(lines);
    auto engine = impl::Engine::create(std::move(engineData));
    return std::make_shared<RegEx>(std::move(engine), RegEx::PrivateTag{});
}

}
