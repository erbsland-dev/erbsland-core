// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Operation.hpp"

#include <erbsland/all.hpp>

namespace app::charset {

/// Registration and built-in cases for one measured operation.
/// @notest{Covered by the character-set profiling coverage test.}
struct OperationDescriptor {
    Operation operation{};  ///< The operation dispatched by the worker.
    el::String id;          ///< Stable functionality identifier.
    el::String description; ///< Human-readable operation description.
    el::StringList api;     ///< Public APIs covered by this path.
    el::StringList cases;   ///< Built-in representative input cases.
};

/// Access the complete `CharSet` profiling registry.
/// @return Every measured operation and its public API coverage.
/// @notest{Covered by the character-set profiling coverage test.}
[[nodiscard]] auto operationDescriptors() -> const el::List<OperationDescriptor> &;

}
