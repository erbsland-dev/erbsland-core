// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if !defined(_WIN32)
#error "InputRecordEraseGuard is only available on Windows."
#endif

#include "../../mem/impl/SecureErase.hpp"

#include <windows.h>

#include <span>

namespace erbsland::cterm::impl {

/// Securely erase one Windows console input record when leaving its scope.
/// @tested{WindowsBackendTest}
class InputRecordEraseGuard final {
public:
    explicit InputRecordEraseGuard(INPUT_RECORD &record) noexcept : _record{record} {}
    ~InputRecordEraseGuard() { mem::impl::secureErase(std::as_writable_bytes(std::span{&_record, 1U})); }

    InputRecordEraseGuard(const InputRecordEraseGuard &) = delete;
    InputRecordEraseGuard(InputRecordEraseGuard &&) = delete;
    auto operator=(const InputRecordEraseGuard &) -> InputRecordEraseGuard & = delete;
    auto operator=(InputRecordEraseGuard &&) -> InputRecordEraseGuard & = delete;

private:
    INPUT_RECORD &_record; ///< Borrowed console record.
};

}
