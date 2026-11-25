// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HeaderDeclarations.hpp"

#include <tool/frame/ProjectTextFile.hpp>

#include <memory>

namespace tool::headers {

/// A worker-owned input snapshot and optional pre-parsed publication declarations.
struct HeaderReadResult final {
    frame::ProjectTextFile file;                            ///< Immutable file text and identity metadata.
    std::shared_ptr<const HeaderDeclarations> declarations; ///< Parsed declarations for selected source headers.
};

}
