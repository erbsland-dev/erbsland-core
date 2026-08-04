// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegEx.hpp"

#include "impl/compiler/Compiler.hpp"
#include "impl/engine/Engine.hpp"

#include "../text/StringCharReader.hpp"
#include "../text/StringConverter.hpp"

#include <atomic>
#include <mutex>

namespace erbsland::re {

RegEx::LazyState::LazyState(const Flags sourceFlags, Settings sourceSettings) noexcept :
    flags{sourceFlags}, settings{std::move(sourceSettings)} {
}

auto RegEx::compile(text::AnyString pattern, const Flags flags, const Settings &settings) -> RegExPtr {
    auto engine = buildEngine(pattern, flags, settings);
    return std::make_shared<RegEx>(std::move(engine), std::move(pattern), PrivateTag{});
}

auto RegEx::lazyCompile(text::AnyString pattern, const Flags flags, const Settings &settings) -> RegExPtr {
    return std::make_shared<RegEx>(std::move(pattern), flags, settings, PrivateTag{});
}

auto RegEx::buildEngine(const text::AnyString &pattern, const Flags flags, const Settings &settings)
    -> impl::ConstEnginePtr {
    auto reader = text::StringCharReader{pattern};
    impl::Compiler compiler{std::move(reader), impl::GroupFlags::fromPatternFlags(flags), settings};
    auto engine = compiler.buildEngine();
    if (flags.isSet(Flag::CRLF)) {
        engine->setInitialFlag(impl::EngineFlag::FoldCRLF);
    }
    return engine;
}

void RegEx::compileNow() const {
    ensureEngineCompiled();
}

auto RegEx::isCompiled() const noexcept -> bool {
    return _lazyState == nullptr || _lazyState->isCompiled.load(std::memory_order_acquire);
}

void RegEx::ensureEngineCompiled() const {
    if (_lazyState != nullptr) {
        const auto &state = _lazyState;
        std::call_once(state->compileOnce, [this, &state]() -> void {
            state->engine = buildEngine(_pattern, state->flags, state->settings);
            state->isCompiled.store(true, std::memory_order_release);
        });
    }
}

auto RegEx::engine() const -> const impl::ConstEnginePtr & {
    ensureEngineCompiled();
    if (_lazyState != nullptr) {
        return _lazyState->engine;
    }
    return _engine;
}

RegEx::RegEx(impl::ConstEnginePtr engine, text::AnyString pattern, PrivateTag) noexcept :
    _engine{std::move(engine)}, _pattern{std::move(pattern)} {
}

RegEx::RegEx(text::AnyString pattern, const Flags flags, Settings settings, PrivateTag) :
    _pattern{std::move(pattern)}, _lazyState{std::make_shared<LazyState>(flags, std::move(settings))} {
}

}
