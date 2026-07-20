// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Source.hpp"
#include "SourceResolverContext.hpp"

#include <memory>

namespace erbsland::conf {

class SourceResolver;
using SourceResolverPtr = std::shared_ptr<SourceResolver>;

/// The interface for any source resolver implementation.
/// @tested{ParserIncludeTest FileSourceResolverTest}
class SourceResolver {
public:
    /// Default destructor.
    virtual ~SourceResolver() = default;

public:
    /// Resolve Sources
    /// This function is called when the parser encounters an <code>include</code> meta-command.
    /// The raw and unprocessed text of the command and the source of the parsed document are given as
    /// arguments to this function.
    /// This function must either return a list of sources that match the expression or throw a `ConfError` exception.
    /// If an exception is thrown, the parsing will be stopped and the thrown exception will be passed to the
    /// caller of `parse()`.
    /// If a list is returned, the parser will parse the sources in the given order and include the parsed
    /// contents in the document. The returned sources should be in a closed state. The parser will open them in
    /// the sequence they are parsed.
    /// @param context The resolve context.
    /// @return A list of sources (see @c SourceList) to include in the document.
    /// @throws ConfError Throw @c ConfError with @c ConfErrorCategory::Syntax if the include text does not match the
    /// required format.
    virtual auto resolve(const SourceResolverContext &context) -> SourceListPtr = 0;
};

}
