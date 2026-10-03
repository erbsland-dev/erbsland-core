// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>

namespace tool::headers {

/// The effective publication policy for one explicitly configured source directory.
struct DirectoryPolicy {
    el::Path directory;                    ///< Source-base-relative directory.
    el::String sourceNamespace;            ///< Fully qualified declaration namespace.
    el::Path targetDirectory;              ///< Canonical include directory; empty means the include root.
    el::String targetNamespace;            ///< Optional namespace alias in the base namespace.
    bool flatten = false;                  ///< Import the namespace, or the selected symbols, into the base namespace.
    el::StringList includedHeaders;        ///< Filename-only selection globs.
    el::StringList excludedHeaders;        ///< Filename-only publication exclusions.
    el::StringList excludedFromFlattening; ///< Filename-only canonical-path exclusions.
    el::StringList excludedFromAll;        ///< Filename-only umbrella exclusions.
    el::StringList includedSymbols;        ///< Explicit exports instead of a namespace-wide import.
};

}
