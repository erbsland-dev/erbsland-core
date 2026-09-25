// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PackageApplication.hpp"

#include <erbsland/core/Definitions.hpp>

#if defined(ERBSLAND_OS_WINDOWS)
auto wmain(const int argc, wchar_t *argv[]) -> int {
    return erbsland::package::PackageApplication{argc, argv}.run();
}
#else
auto main(const int argc, char *argv[]) -> int {
    return erbsland::package::PackageApplication{argc, argv}.run();
}
#endif
