// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::cryptology::impl {

/// Securely erase local scratch storage when its scope ends.
/// @tparam T A type with a `secureErase()` operation.
/// @tested{PasswordPrimitiveTest}
template <typename T>
class SecureEraseGuard final {
public:
    /// Protect one local scratch value.
    explicit SecureEraseGuard(T &value) noexcept : _value{&value} {}

    /// Securely erase the guarded scratch value.
    ~SecureEraseGuard() noexcept {
        if (_value != nullptr) {
            _value->secureErase();
        }
    }

    // defaults/deletions
    SecureEraseGuard(const SecureEraseGuard &) = delete;
    SecureEraseGuard(SecureEraseGuard &&) = delete;
    auto operator=(const SecureEraseGuard &) -> SecureEraseGuard & = delete;
    auto operator=(SecureEraseGuard &&) -> SecureEraseGuard & = delete;

public:
    /// Stop guarding the value, for example immediately before returning the guarded value.
    void release() noexcept { _value = nullptr; }

private:
    T *_value; ///< The scratch value erased when this guard is destroyed.
};

}
