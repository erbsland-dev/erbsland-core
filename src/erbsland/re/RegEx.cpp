// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RegEx.hpp"

#include "impl/compiler/Compiler.hpp"
#include "impl/engine/Engine.hpp"

#include "../text/StringCharReader.hpp"

namespace erbsland::re {

auto RegEx::compile(const text::String &pattern, const Flags flags, Settings settings) -> RegExPtr {
    return compileReader(text::StringCharReader{pattern}, flags, std::move(settings));
}

auto RegEx::compile(const text::U16String &pattern, const Flags flags, Settings settings) -> RegExPtr {
    return compileReader(text::StringCharReader{pattern}, flags, std::move(settings));
}

auto RegEx::compile(const text::U32String &pattern, const Flags flags, Settings settings) -> RegExPtr {
    return compileReader(text::StringCharReader{pattern}, flags, std::move(settings));
}

auto RegEx::compileReader(text::StringCharReader reader, const Flags flags, Settings settings) -> RegExPtr {
    impl::Compiler compiler{std::move(reader), impl::GroupFlags::fromPatternFlags(flags), std::move(settings)};
    auto engine = compiler.buildEngine();
    if (flags.isSet(Flag::CRLF)) {
        engine->setInitialFlag(impl::EngineFlag::FoldCRLF);
    }
    return std::make_shared<RegEx>(engine, PrivateTag{});
}

auto RegEx::engine() const noexcept -> const impl::ConstEnginePtr & {
    return _engine;
}

RegEx::RegEx(impl::ConstEnginePtr engine, PrivateTag) noexcept : _engine{std::move(engine)} {
}

}
