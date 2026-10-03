// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

namespace tool::headers {

/// A namespace-scope type declaration found without interpreting C++ bodies.
struct HeaderDeclaration {
    el::String sourceNamespace; ///< Fully qualified namespace.
    el::String name;            ///< Unqualified type or alias name.
    el::String forwardText;     ///< Safe standalone declaration, empty when extraction is unsupported.
};

}
