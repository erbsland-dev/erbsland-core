// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteRange.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/String.hpp>

namespace tool::github_workflows {

/// A single external uses value and the exact text span that may be replaced.
struct ActionReference {
    el::ItemIndex line;       ///< Physical workflow line index.
    el::ByteRange valueRange; ///< Unquoted action@ref bytes within the original line.
    el::String indent;        ///< Original indentation for an adjacent annotation.
    el::String action;        ///< Complete owner/repository[/subpath] action name.
    el::String repository;    ///< owner/repository for remote tag lookup.
    el::String ref;           ///< Existing pin or other reference, possibly empty.
};

}
