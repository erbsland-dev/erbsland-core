// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionErrorContext_fwd.hpp"
#include "OptionManager_fwd.hpp"
#include "OptionResult.hpp"
#include "Options_fwd.hpp"

#include "../core/CommandLineArguments.hpp"
#include "../stream/TextOutputStream_fwd.hpp"
#include "../text/TextDocument.hpp"

namespace erbsland::options {

/// The option manager orchestrates parsing and validation of command line options.
/// Names use canonical dash-separated ASCII words; single-dash short options remain case-sensitive.
/// See the options reference for syntax, normalization, validation, and built-in requests.
/// @seedoc{/reference/options/command_line_options}
/// @tested{OptionsParserTest}
class OptionManager {
public:
    /// Create an option manager with an empty options root.
    ///
    /// The created root contains built-in help and version options.
    OptionManager();
    /// Create an option manager for an options root.
    /// @param options The root options definition. If null, parsing behaves as if an empty root was supplied.
    explicit OptionManager(OptionsPtr options);

    // defaults
    ~OptionManager() = default;
    OptionManager(const OptionManager &) = default;
    auto operator=(const OptionManager &) -> OptionManager & = default;
    OptionManager(OptionManager &&) = default;
    auto operator=(OptionManager &&) -> OptionManager & = default;

public: // accessors
    /// Get the options root.
    [[nodiscard]] auto options() const noexcept -> const OptionsPtr & { return _options; }

public:
    /// Parse already converted command line arguments.
    /// Calls all registered pre-hooks before parsing.
    /// Calls the affected post-hooks after parsing.
    /// Returns after successful and erroneous parsing and if `--help` or `--version` is encountered.
    /// You are responsible to handle displaying help, version, and errors, based on the returned `OptionResult`.
    /// Sensitive option text is replaced in `args` before this method returns.
    /// @param args The mutable command line arguments to parse and mask.
    /// @return The result of the parsing operation.
    [[nodiscard]] auto parse(core::CommandLineArguments &args) -> OptionResult;
    /// Parse already converted command line arguments or throw on error.
    /// Calls all registered pre-hooks before parsing.
    /// Calls the affected post-hooks after parsing.
    /// Automatically calls `displayVersion` or `displayHelp` if the respective option is encountered and returns
    /// a null pointer to indicate successful parsing without values.
    /// Automatically displays an error document before throwing.
    /// Sensitive option text is replaced in `args` before this method returns.
    /// @param args The mutable command line arguments to parse and mask.
    /// @return The parsed option values or a null pointer if help or version was displayed.
    /// @throws options::OptionError If parsing, validation, or a callback fails.
    [[nodiscard]] auto parseOrThrow(core::CommandLineArguments &args) -> OptionValuesPtr;
    /// Display help using the configured renderer.
    /// Help is displayed using the configured renderer.
    /// The default renderer writes the output to the terminal or standard output.
    /// @param moduleName The name of the module to display help for. Empty for main help.
    void displayHelp(const text::String &moduleName) const;
    /// Display detailed help or matching topics for a module, option or choice.
    /// @param moduleName Selected module name, or empty for root search order.
    /// @param helpName Raw detailed-help target.
    void displayDetailedHelp(const text::String &moduleName, const text::String &helpName) const;
    /// Display the reduced module overview.
    void displayModuleOverview() const;
    /// Display version information using the configured renderer.
    /// Version is displayed using the configured renderer.
    /// The default renderer writes the output to the terminal or standard output.
    /// @param moduleName The name of the module to display help for. Empty for main help.
    void displayVersion(const text::String &moduleName) const;
    /// Display an error message.
    /// The error message is displayed using the configured renderer.
    /// @param errorContext Structured parser or validation error details.
    void displayError(const OptionErrorContext &errorContext) const;
    /// Build the help document.
    /// @param moduleName The selected module name, or empty for root help.
    /// @return A neutral document tree that can be rendered as plain text or terminal output.
    [[nodiscard]] auto helpDocument(const text::String &moduleName) const -> text::TextDocument;
    /// Build detailed help or matching topics for a module, option or choice.
    /// @param moduleName Selected module name, or empty for root search order.
    /// @param helpName Raw detailed-help target.
    /// @return A neutral detail page or compact list of matching help topics.
    /// @throws OptionError If the selector is malformed or no visible help target matches.
    [[nodiscard]] auto detailedHelpDocument(const text::String &moduleName, const text::String &helpName) const
        -> text::TextDocument;
    /// Build the reduced module overview.
    /// @return A neutral document with only module usage and visible modules.
    [[nodiscard]] auto moduleOverviewDocument() const -> text::TextDocument;
    /// Build the version document.
    /// @param moduleName The selected module name, or empty for root version output.
    /// @return A neutral document tree with application version information.
    [[nodiscard]] auto versionDocument(const text::String &moduleName) const -> text::TextDocument;
    /// Build an option error document.
    /// @param errorContext Structured parser or validation error details.
    /// @return A neutral document tree with the diagnostic message and optional source context.
    [[nodiscard]] auto errorDocument(const OptionErrorContext &errorContext) const -> text::TextDocument;

public: // convenience overloads
    /// Parse UTF-8 encoded command line arguments.
    /// @param argc Argument count from `main`.
    /// @param argv UTF-8 encoded argument vector from `main`.
    /// @return The result of parsing.
    /// @see parse(CommandLineArguments&)
    /// @see convertCommandLineArguments(int, char**)
    [[nodiscard]] auto parse(int argc, char *argv[]) -> OptionResult;
    /// Parse wide command line arguments.
    /// @param argc Argument count from `wmain`.
    /// @param argv Wide argument vector from `wmain`.
    /// @return The result of parsing.
    /// @see parse(CommandLineArguments&)
    /// @see convertCommandLineArguments(int, wchar_t**)
    [[nodiscard]] auto parse(int argc, wchar_t *argv[]) -> OptionResult;
    /// Parse UTF-8 encoded command line arguments or throw on error.
    /// @param argc Argument count from `main`.
    /// @param argv UTF-8 encoded argument vector from `main`.
    /// @return The parsed option values or null if help/version was displayed.
    /// @throws options::OptionError If parsing, validation, or a callback fails.
    /// @see parseOrThrow(CommandLineArguments&)
    /// @see convertCommandLineArguments(int, char**)
    [[nodiscard]] auto parseOrThrow(int argc, char *argv[]) -> OptionValuesPtr;
    /// Parse wide command line arguments or throw on error.
    /// @param argc Argument count from `wmain`.
    /// @param argv Wide argument vector from `wmain`.
    /// @return The parsed option values or null if help/version was displayed.
    /// @throws options::OptionError If parsing, validation, or a callback fails.
    /// @see parseOrThrow(CommandLineArguments&)
    /// @see convertCommandLineArguments(int, wchar_t**)
    [[nodiscard]] auto parseOrThrow(int argc, wchar_t *argv[]) -> OptionValuesPtr;

public: // conversion
    /// Convert UTF-8 command line arguments to library strings.
    /// Assumes UTF-8 encoding. Uses tolerant decoding using the replacement character for invalid sequences.
    /// @param argc Argument count from `main`.
    /// @param argv UTF-8 encoded argument vector from `main`.
    /// @return A converted command-line argument list using Erbsland Core strings.
    [[nodiscard]] static auto convertCommandLineArguments(int argc, char *argv[]) -> core::CommandLineArguments;
    /// Convert wide command line arguments to library strings.
    /// Assumes UTF-16/32 encoding. Uses tolerant decoding using the replacement character for invalid sequences.
    /// @param argc Argument count from `wmain`.
    /// @param argv Wide argument vector from `wmain`.
    /// @return A converted command-line argument list using Erbsland Core strings.
    /// @note This method is designed for Windows processes, that supply `wchar_t` arguments via main, which
    ///     is a safer alternative to the more unpredictable `char` encoding.
    [[nodiscard]] static auto convertCommandLineArguments(int argc, wchar_t *argv[]) -> core::CommandLineArguments;

private:
    /// Render a text document to plain-text output.
    static void renderPlainDocument(const text::TextDocument &document, const stream::TextOutputStreamPtr &output);

    OptionsPtr _options; ///< The options root.
};

}
