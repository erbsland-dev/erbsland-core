// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../text/StringView.hpp"

namespace erbsland::options::impl {

/// Extract the executable name from an unprocessed command line path.
[[nodiscard]] auto extractExecutableName(const text::StringView &executablePath) -> text::String;

}
