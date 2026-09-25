// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceCompiler.hpp"

#include <erbsland/core/Definitions.hpp>

#if defined(ERBSLAND_OS_WINDOWS)
auto wmain(const int argc, wchar_t *argv[]) -> int {
    auto application = erbsland::resource::compiler::ResourceCompiler{argc, argv};
    return application.run();
}
#else
auto main(const int argc, char *argv[]) -> int {
    auto application = erbsland::resource::compiler::ResourceCompiler{argc, argv};
    return application.run();
}
#endif
