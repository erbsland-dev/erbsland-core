// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../AccessCheck.hpp"
#include "../../FileAccessCheck.hpp"
#include "../../FileSourceResolver.hpp"
#include "../../SignatureValidator.hpp"

namespace erbsland::conf::impl {

/// Internal settings of the parser.
/// @needtest{Covered indirectly by parser integration tests.}
struct ParserSettings {
    /// The source resolver.
    /// If set, this function is used when the parser resolves references to additional sources.
    /// If this variable contains a `nullptr`, the `\@include` statement is disabled.
    SourceResolverPtr sourceResolver = std::make_shared<FileSourceResolver>();

    /// An object that verifies if a configuration can be read from a given source.
    /// If set, each source is checked right before it is opened for reading.
    AccessCheckPtr accessCheck = std::make_shared<FileAccessCheck>();

    /// An object with the implementation for signature verification.
    /// If `nullptr` the parser rejects signed documents by default.
    /// If set, all documents, even these without `\@signature` must be checked by this object.
    SignatureValidatorPtr signatureValidator;
};

}
