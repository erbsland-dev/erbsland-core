// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/StringView.hpp"
#include "../../unit/Version.hpp"

namespace erbsland::core::impl {

auto libraryVersion() noexcept -> unit::Version;
auto libraryVersionText() noexcept -> text::StringView;

}
