// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WindowsLocalBuffer.hpp"

namespace erbsland::cryptology::impl {

WindowsLocalBuffer::~WindowsLocalBuffer() {
    if (_data != nullptr) {
        if (_sensitive) {
            // Microsoft DPAPI-NG returns plaintext in LocalAlloc storage; erase its full reported extent before free.
            SecureZeroMemory(_data, _size);
        }
        LocalFree(_data);
    }
}

}
