// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#if defined(_WIN32)
#define PACKAGE_EXPORT __declspec(dllexport)
#else
#define PACKAGE_EXPORT
#endif

extern "C" PACKAGE_EXPORT auto packageLibraryValue() -> int {
    return 42;
}
