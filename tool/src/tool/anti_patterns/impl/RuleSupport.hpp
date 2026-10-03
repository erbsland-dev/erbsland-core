// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../SourceIndex.hpp"

namespace tool::anti_patterns::impl {

/// Test whether a block belongs to a namespace-only scope chain.
[[nodiscard]] auto namespaceType(const SourceIndex &source, const SourceBlock &block) -> bool;
/// Build the qualified namespace name enclosing a block.
[[nodiscard]] auto namespaceName(const SourceIndex &source, const SourceBlock &block, bool includeBlock = false)
    -> el::String;
/// Check a namespace against the library source directory.
[[nodiscard]] auto matchesSourceDirectory(const SourceIndex &source, const el::String &name) -> bool;
/// Detect a specialization of a standard-library extension point.
[[nodiscard]] auto standardExtension(const SourceIndex &source, el::ByteIndex offset, const el::String &declaration)
    -> bool;
/// Detect a preceding API documentation block.
[[nodiscard]] auto hasApiDocumentation(const SourceIndex &source, el::ByteIndex start) -> bool;
/// Detect an out-of-class member implementation.
[[nodiscard]] auto outOfClassMember(const SourceIndex &source, el::ByteIndex start) -> bool;
/// Detect a template, constraint, attribute, or test metadata preamble.
[[nodiscard]] auto declarationPreamble(const el::String &line) -> bool;
/// Detect a declaration continuing a template preamble.
[[nodiscard]] auto templateContinuation(const SourceIndex &source, el::LineIndex lineIndex) -> bool;
/// Detect a declaration continuing a requires clause.
[[nodiscard]] auto constraintContinuation(const SourceIndex &source, el::LineIndex lineIndex) -> bool;

}
