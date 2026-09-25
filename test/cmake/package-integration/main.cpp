// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#if defined(PACKAGE_CLIENT)
extern "C" auto packageLibraryValue() -> int;
#endif
#if defined(PACKAGE_FRAMEWORK)
extern "C" auto packageFrameworkValue() -> int;
#endif

auto main() -> int {
#if defined(PACKAGE_CLIENT)
    if (packageLibraryValue() != 42) { return 1; }
#endif
#if defined(PACKAGE_FRAMEWORK)
    if (packageFrameworkValue() != 7) { return 2; }
#endif
#if defined(PACKAGE_CLIENT) || defined(PACKAGE_FRAMEWORK)
    return 0;
#else
    return 0;
#endif
}
