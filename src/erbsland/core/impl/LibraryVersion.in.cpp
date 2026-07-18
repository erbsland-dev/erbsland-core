// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LibraryVersion.hpp"

namespace erbsland::core::impl {

using namespace text::literals;

auto libraryVersion() noexcept -> unit::Version {
    // clang-format off
    return unit::Version{
        @ERBSLAND_CORE_VERSION_MAJOR@U,
        @ERBSLAND_CORE_VERSION_MINOR@U,
        @ERBSLAND_CORE_VERSION_REVISION@U,
        @ERBSLAND_CORE_VERSION_BUILD@U};
    // clang-format on
}

auto libraryVersionText() noexcept -> text::String {
    return "@ERBSLAND_CORE_VERSION_TEXT@"_el;
}

}
