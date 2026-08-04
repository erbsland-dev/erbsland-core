// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::path::impl {

/// Describe the native symbolic-link reparse payload.
struct WindowsSymbolicLinkReparseData {
    std::uint32_t reparseTag;
    std::uint16_t reparseDataLength;
    std::uint16_t reserved;
    std::uint16_t substituteNameOffset;
    std::uint16_t substituteNameLength;
    std::uint16_t printNameOffset;
    std::uint16_t printNameLength;
    std::uint32_t flags;
    wchar_t pathBuffer[1];
};

}
