// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompilerApplication.hpp"

#include <erbsland/Definitions.hpp>

#if defined(ERBSLAND_OS_WINDOWS)
auto wmain(const int argc, wchar_t *argv[]) -> int {
#else
auto main(const int argc, char *argv[]) -> int {
#endif
    auto application = erbsland::i18n::compiler::CompilerApplication{argc, argv};
    return application.run();
}
