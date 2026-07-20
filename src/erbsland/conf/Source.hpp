// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfError.hpp"
#include "Source_fwd.hpp"
#include "SourceIdentifier.hpp"

#include "impl/utilities/InternalView.hpp"

#include "../path/Path.hpp"
#include "../text/CodeSnippet.hpp"
#include "../text/String.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace erbsland::conf {

using SourceList = std::vector<SourcePtr>;
using SourceListPtr = std::shared_ptr<SourceList>;

/// Interface for the data source to read the configuration.
/// Implementation notes:
/// - Constructing instances of source subclasses should be a lightweight operation,
///     as sources may be created in batches, e.g. when an `\@include` directive with a recursive pattern is
///     encountered.
/// - The constructor of a source shouldn't throw exceptions, unless a program termination due to internal
///     errors is favourable.
/// - Heavy allocations and API calls shall be made in the `open()` method.
/// - Any IO exceptions shall be raised in the `open()` and/or `readLine()` methods.
/// @tested{StringSourceTest TextStreamSourceTest FileSourceTest SourceCreateTest}
class Source {
public:
    /// Default destructor.
    virtual ~Source() = default;

public: // accessors
    /// Get the name of the source.
    /// The name of the source also specifies its type or protocol. In a source identifier, the source name is
    /// separated from the source path by a colon.
    /// @return The name of the source.
    [[nodiscard]] auto name() const noexcept -> text::String { return identifier()->name(); }
    /// Get the path of the source.
    /// The path of the source specifies the location of the source. In a source identifier, the source path is
    /// separated from the source name by a colon.
    /// @return The path of the source.
    [[nodiscard]] auto path() const noexcept -> text::String { return identifier()->path(); }
    /// Get the source identifier.
    /// @return The source identifier.
    [[nodiscard]] virtual auto identifier() const noexcept -> SourceIdentifierPtr = 0;

public: // working with the source.
    /// Open the source.
    /// The open method is only called **once** in the lifetime of a source.
    /// After a successful call of `open()`, the method `isOpen()` must return `true`.
    /// @throws ConfError (IO) If an error occurs while opening the source.
    virtual void open() = 0;
    /// Test if the source is open.
    /// @return `true` if the source is open, `false` otherwise.
    [[nodiscard]] virtual auto isOpen() const noexcept -> bool = 0;
    /// Test if the source reached its end.
    /// @return `true` if the source reached its end, `false` otherwise.
    [[nodiscard]] virtual auto atEnd() const noexcept -> bool = 0;
    /// Reads a line from the source.
    /// The read line must contain the ending newline sequence if there is any.
    /// The returned string is an owning COW value and remains valid after subsequent reads and after this source is
    /// destroyed. An empty string signals the end of the source.
    /// @return The next line, or an empty string if no more data is available.
    /// @throws ConfError (IO) If an error occurs while reading the line.
    [[nodiscard]] virtual auto readLine() -> text::String = 0;
    /// Get a best-effort source excerpt around a location.
    /// A stream source may read up to two additional context lines if the affected line is already buffered.
    /// @param location The zero-based source location.
    /// @return Up to two context lines around the affected line, if available.
    [[nodiscard]] virtual auto codeSnippet(unit::CodeLocation location) noexcept
        -> std::optional<text::CodeSnippet> = 0;
    /// Closes the source.
    /// Closes the source and releases any system resources associated with the source.
    /// After a call of `close()`, the method `isOpen()` must return `false`.
    virtual void close() noexcept = 0;

public:
    /// Create a source for a file path.
    /// The returned source does not open the file immediately. The file is
    /// opened on the first call to `open()`.
    /// @param path The path to the file.
    [[nodiscard]] static auto fromFile(path::Path path) noexcept -> SourcePtr;
    /// Create a source from the given UTF-8 encoded string.
    /// @param text The string with the text. The source shares ownership of the string data.
    [[nodiscard]] static auto fromString(text::String text) noexcept -> SourcePtr;

public: // testing
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const Source &object) -> impl::InternalViewPtr;
#endif
};

}
