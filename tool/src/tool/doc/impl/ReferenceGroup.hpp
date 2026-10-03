// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>
#include <erbsland/StringList.hpp>

namespace tool::doc::impl {

/// One explicitly configured group of public reference headers.
struct ReferenceGroup {
    el::Path page;          ///< Relative reference page.
    el::String title;       ///< Page title, or an empty generated-title request.
    el::StringList headers; ///< Expanded source-relative header paths.
};

}
