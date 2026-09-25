// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AccessCheck_fwd.hpp"
#include "Document.hpp"
#include "SignatureValidator.hpp"
#include "Source.hpp"
#include "SourceResolver.hpp"

#include "impl/parser/ParserSettings.hpp"

#include "../text/placeholder/Filter.hpp"
#include "../text/placeholder/Source.hpp"
#include "../text/StringMap.hpp"

#include <optional>

namespace erbsland::conf {

/// This parser reads the Erbsland Configuration Language.
/// *Multithreading*: This parser is **reentrant**, and therefore it can be used in multiple threads, as long each
/// thread uses an individual instance of the parser.
/// @tested{ParserBasicTest ParserConvenienceTest ParserIncludeTest ParserPlaceholderTest ParserSignatureTest}
class Parser final {
public:
    /// Create a new parser with the default settings.
    Parser() = default;

    // defaults
    ~Parser() = default;

public:
    /// Set a custom source resolver used to resolve include directives while parsing.
    /// By default, an instance of `FileSourceResolver` is used, which supports file-based includes, as specified
    /// in the format recommended in the documentation.
    /// @param sourceResolver The custom source resolver, or `nullptr` to disable the `include` meta-command.
    void setSourceResolver(const SourceResolverPtr &sourceResolver) noexcept;
    /// Set a custom access check.
    /// By default, an instance of `FileAccessCheck` with default options is used. This instance limits included files
    /// to the same directory and subdirectories of the including configuration.
    /// @param accessCheck An instance of a source access check implementation, or `nullptr` to disable
    ///     the `include` meta-command.
    void setAccessCheck(const AccessCheckPtr &accessCheck) noexcept;
    /// Set a signature validator.
    /// By default, no signature validator is set. This allows parsing all unsigned configuration documents.
    /// Documents with a `signature` meta-value get rejected by the parser.
    /// @param signatureValidator An instance of a signature validator implementation, or `nullptr` to
    ///     disable signature validation.
    void setSignatureValidator(const SignatureValidatorPtr &signatureValidator) noexcept;
    /// Add a provider for one or more placeholder sources.
    /// @throws err::ParameterError If `source` is null.
    /// @throws err::LogicError If the provider has no valid names or a name is already registered.
    void addPlaceholderSource(const text::placeholder::SourcePtr &source);
    /// Remove a placeholder source provider. A provider that is not registered is ignored.
    void removePlaceholderSource(const text::placeholder::SourcePtr &source) noexcept;
    /// Add a provider for one or more placeholder filters.
    /// @throws err::ParameterError If `filter` is null.
    /// @throws err::LogicError If the provider has no valid names or a name is already registered.
    void addPlaceholderFilter(const text::placeholder::FilterPtr &filter);
    /// Remove a placeholder filter provider. A provider that is not registered is ignored.
    void removePlaceholderFilter(const text::placeholder::FilterPtr &filter) noexcept;
    /// Add the built-in environment placeholder source under `env` or a custom name.
    /// @param name The source name, or empty to use `env`.
    /// @throws err::LogicError If the name is invalid or already registered.
    void addPlaceholderEnvironmentSource(const text::String &name = {});
    /// Set the variables for the built-in variable placeholder source and add it if needed.
    /// Variable names use regular ELCL name normalization and are therefore case-insensitive, with spaces and
    /// underscores treated as equivalent.
    /// @param variables The replacement text indexed by variable name.
    /// @param name The source name, or empty to use `var`.
    /// @throws ConfError If a variable or source name is invalid.
    /// @throws err::LogicError If another provider already registered the source name.
    void setPlaceholderVariableSource(text::StringMap<text::String> variables, const text::String &name = {});
    /// Add the built-in text placeholder filters.
    /// @throws err::LogicError If any filter name is already registered.
    void addPlaceholderTextFilters();
    /// Parse the given source into a configuration document and throw an exception on any error.
    /// @param source The source to parse. Should be closed.
    /// @return The root node of the parsed configuration tree.
    /// @throws ConfError if there was any problem with the parsed source or document.
    /// @throws err::ParameterError if `source` is `nullptr`.
    auto parseOrThrow(const SourcePtr &source) -> DocumentPtr;
    /// Parse the given source into a configuration document.
    /// @param source The source to parse. Should be closed.
    /// @return The root node of the parsed configuration tree or nullptr on any parsing error.
    ///     Use `lastError()` to access the last error.
    /// @throws err::ParameterError if `source` is `nullptr`.
    auto parse(const SourcePtr &source) -> DocumentPtr;
    /// Access the last error.
    /// @return The context of the last error, or an empty context if no parse error is available.
    auto lastError() const noexcept -> ConfErrorContext;

public: // convenience methods
    /// @name Convenience Methods
    /// These methods are convenience methods to call `parse` or `parseOrThrow`.
    /// They construct a `Source` from the given parameters and call `parse` or `parseOrThrow` using this source.
    /// @{

    /// Parse the given file into a configuration document and throw an exception on error.
    [[nodiscard]] auto parseFileOrThrow(path::Path path) -> DocumentPtr {
        return parseOrThrow(Source::fromFile(std::move(path)));
    }
    /// Parse the given file into a configuration document and return a null pointer on error.
    [[nodiscard]] auto parseFile(path::Path path) -> DocumentPtr { return parse(Source::fromFile(std::move(path))); }
    /// Parse the given text into a configuration document and throw an exception on error.
    [[nodiscard]] auto parseTextOrThrow(text::String text) -> DocumentPtr {
        return parseOrThrow(Source::fromString(std::move(text)));
    }
    /// Parse the given text into a configuration document and return a null pointer on error.
    [[nodiscard]] auto parseText(text::String text) -> DocumentPtr {
        return parse(Source::fromString(std::move(text)));
    }
    /// @}

private:
    impl::ParserSettings _settings;             ///< The parser settings.
    std::optional<ConfErrorContext> _lastError; ///< The last error that occurred.
};

}
