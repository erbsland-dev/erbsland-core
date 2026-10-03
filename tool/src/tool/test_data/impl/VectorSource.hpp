// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/List.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>

namespace tool::test_data::impl {

/// Identify a supported source independently of its configured location and checksum.
struct VectorSource {
    el::String id;          ///< Configuration section identifier.
    el::String filename;    ///< Offline source filename.
    el::StringList outputs; ///< Prepared response paths, validated before loading sources.
    el::String family;      ///< Family selected on the command line.
    /// Get the supported catalog in stable import order.
    [[nodiscard]] static auto all() -> const el::List<VectorSource> &;
};

}
