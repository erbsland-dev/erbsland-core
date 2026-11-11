// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>

namespace demo {

/// An owned batch delivered to the main loop, optionally completing its search.
struct SearchUpdate final {
    el::PathList paths;     ///< Absolute paths, in traversal order.
    bool finished{false};   ///< No more batches will follow for this search.
    bool incomplete{false}; ///< Some filesystem entries could not be inspected.
};

}
