// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsProtectedDataApi.hpp"

#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

WindowsProtectedDataApi::WindowsProtectedDataApi() {
    // Windows loader contract: resolve the optional DPAPI-NG dependency only when the platform provider is selected,
    // and constrain the lookup to the trusted Windows system directory.
    _module = LoadLibraryExW(L"ncrypt.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (_module == nullptr) {
        throw CryptologyError{"The Windows DPAPI-NG library is unavailable."_el};
    }

    // Microsoft DPAPI-NG API contract: all four operations are required for the provider lifetime. Keep their lookup
    // explicit so a security review can compare this list directly with the provider's native calls.
    _createProtectionDescriptorFn =
        reinterpret_cast<CreateProtectionDescriptorFn>(GetProcAddress(_module, "NCryptCreateProtectionDescriptor"));
    _closeProtectionDescriptorFn =
        reinterpret_cast<CloseProtectionDescriptorFn>(GetProcAddress(_module, "NCryptCloseProtectionDescriptor"));
    _protectSecretFn = reinterpret_cast<ProtectSecretFn>(GetProcAddress(_module, "NCryptProtectSecret"));
    _unprotectSecretFn = reinterpret_cast<UnprotectSecretFn>(GetProcAddress(_module, "NCryptUnprotectSecret"));
    if (_createProtectionDescriptorFn == nullptr || _closeProtectionDescriptorFn == nullptr ||
        _protectSecretFn == nullptr || _unprotectSecretFn == nullptr) {
        FreeLibrary(_module);
        _module = nullptr;
        throw CryptologyError{"The Windows DPAPI-NG library is missing required entry points."_el};
    }
}

WindowsProtectedDataApi::~WindowsProtectedDataApi() {
    if (_module != nullptr) {
        FreeLibrary(_module);
    }
}

}
