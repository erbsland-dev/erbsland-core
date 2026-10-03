// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Backend.hpp"

#include "impl/VirtualBackend.hpp"

#ifdef ERBSLAND_OS_WINDOWS
#include "impl/WindowsBackend.hpp"
#else
#include "impl/PosixBackend.hpp"
#endif

namespace erbsland::cterm {

auto Backend::createPlatformDefault(const TerminalFlags terminalFlags) -> BackendPtr {
    if (terminalFlags.has(TerminalFlag::VirtualTty)) {
        return std::make_shared<impl::VirtualBackend>();
    }
#ifdef ERBSLAND_OS_WINDOWS
    return impl::WindowsBackend::getOrCreate(terminalFlags);
#else
    return impl::PosixBackend::getOrCreate(terminalFlags);
#endif
}

}
