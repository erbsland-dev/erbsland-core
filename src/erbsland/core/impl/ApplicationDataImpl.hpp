// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationData.hpp"
#include "EventData_fwd.hpp"

#include "../../resource/ResourceManager_fwd.hpp"
#include "../../stream/TextOutputStream_fwd.hpp"

#include <atomic>
#include <memory>
#include <mutex>

namespace erbsland::core::impl {

/// The default storage implementation for the internal application data.
/// @tested{ApplicationOptionsTest ApplicationPartApplicationTest ApplicationTestScopeTest}
class ApplicationDataImpl : public ApplicationData {
public:
    /// Create the default internal application-data storage.
    ApplicationDataImpl();
    /// Release internal application-data resources.
    ~ApplicationDataImpl() override;

public: // implement ApplicationData
    void setCommandLineArguments(int argc, char *argv[]) override;
    void setCommandLineArguments(int argc, wchar_t *argv[]) override;
    void cleanupBeforeAppExit() noexcept override;
    [[nodiscard]] auto event() -> EventData & override;
    void renderSystemOutput(const text::TextDocument &document) override;

public: // accessors
    [[nodiscard]] auto info() noexcept -> ApplicationInfo & override;
    [[nodiscard]] auto commandLineArguments() const noexcept -> const CommandLineArguments & override;
    [[nodiscard]] auto commandLineArgumentsForParsing() noexcept -> CommandLineArguments & override;
    void maskSensitiveCommandLineText(const options::OptionSensitiveTextLocations &locations) noexcept override;
    [[nodiscard]] auto options() noexcept -> const options::OptionsPtr & override;
    void setOptions(options::OptionsPtr options) noexcept override;
    [[nodiscard]] auto optionValues() noexcept -> const options::OptionValuesPtr & override;
    void setOptionValues(options::OptionValuesPtr optionValues) noexcept override;
    [[nodiscard]] auto systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle & override;
    void setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept override;
    [[nodiscard]] auto initializeFn() noexcept -> const InitializeFn & override;
    void setInitializeFn(InitializeFn initializeFn) noexcept override;
    [[nodiscard]] auto mainFn() noexcept -> const MainFn & override;
    void setMainFn(MainFn mainFn) noexcept override;
    [[nodiscard]] auto partManagerMutex() noexcept -> std::mutex & override;
    [[nodiscard]] auto partManager() noexcept -> const ApplicationPartManagerPtr & override;
    void setPartManager(ApplicationPartManagerPtr manager) noexcept override;
    [[nodiscard]] auto randomMutex() noexcept -> std::mutex & override;
    [[nodiscard]] auto random() noexcept -> const random::RandomPtr & override;
    void setRandom(random::RandomPtr random) noexcept override;
    [[nodiscard]] auto secureRandom() noexcept -> const random::RandomPtr & override;
    void setSecureRandom(random::RandomPtr random) noexcept override;
    [[nodiscard]] auto cryptologyConfiguration() -> cryptology::CryptologyConfiguration & override;
    [[nodiscard]] auto resources() -> const resource::Resources & override;
    [[nodiscard]] auto systemMutex() noexcept -> std::mutex & override;
    [[nodiscard]] auto displayText() noexcept -> const i18n::DisplayTextMapConstPtr & override;
    void setDisplayText(i18n::DisplayTextMapConstPtr displayText) noexcept override;
    [[nodiscard]] auto userLookup() noexcept -> const system::UserLookupPtr & override;
    void setUserLookup(system::UserLookupPtr userLookup) noexcept override;
    [[nodiscard]] auto isTerminalEnabled() const noexcept -> bool override;
    void setTerminalEnabled(bool enabled) noexcept override;
    [[nodiscard]] auto terminal() noexcept -> const cterm::TerminalPtr & override;
    void setTerminal(cterm::TerminalPtr terminal) noexcept override;
    void setStandardStreamRedirect(stream::StandardStreamRedirect redirect) noexcept override;

private:
    template <typename Char>
    /// Mask sensitive native command-line arguments.
    static void maskNativeArguments(
        int argumentCount, Char **arguments, const options::OptionSensitiveTextLocations &locations) noexcept;

    /// Create a plain system output stream for a text document.
    [[nodiscard]] static auto plainSystemOutputStream(const text::TextDocument &document)
        -> stream::TextOutputStreamPtr;

private:
    ApplicationInfo _info;                                     ///< Application metadata.

    std::atomic<bool> _commandLineArgumentsInitialized{false}; ///< If command line arguments are initialized.
    CommandLineArguments _commandLineArguments;                ///< Converted command line arguments.
    int _nativeArgumentCount{0};                               ///< The original native argument count.
    char **_nativeArguments{nullptr};                          ///< Borrowed original UTF-8 argument vector.
    wchar_t **_nativeWideArguments{nullptr};                   ///< Borrowed original wide argument vector.

    options::OptionsPtr _options;                              ///< The global options configuration.
    options::OptionValuesPtr _optionValues;                    ///< The option values after parsing.
    cterm::TerminalDocumentStyle _systemOutputStyle{
        cterm::TerminalDocumentStyle::defaultSystemOutput()};  ///< Style for system output documents.

    InitializeFn _initializeFn;                                ///< Lamda-based override of initialize().
    MainFn _mainFn;                                            ///< Lambda-based override of main().

    std::mutex _partManagerMutex;                              ///< Protects part-manager creation.
    ApplicationPartManagerPtr _partManager;                    ///< Optional detached part manager.

    std::mutex _randomMutex;                                   ///< Mutex for lazy random generator creation.
    random::RandomPtr _random;                                 ///< Shared fast random generator.
    random::RandomPtr _secureRandom;                           ///< Shared secure random generator.

    std::mutex _cryptologyMutex;                               ///< Mutex for lazy cryptology configuration creation.
    std::unique_ptr<cryptology::CryptologyConfiguration> _cryptologyConfiguration; ///< Cryptology configuration.

    std::mutex _resourceMutex;                                   ///< Mutex for lazy resource-manager creation.
    std::unique_ptr<resource::ResourceManager> _resourceManager; ///< Compiled-resource manager.

    std::mutex _systemMutex;                                     ///< Mutex for lazy system service creation.
    i18n::DisplayTextMapConstPtr _displayText;                   ///< Shared application display texts.
    system::UserLookupPtr _userLookup;                           ///< Shared user and group lookup service.

    bool _isTerminalEnabled{false};                              ///< Flag if the terminal was enabled.
    cterm::TerminalPtr _terminal;                                ///< The terminal instance.

    stream::StandardStreamRedirect _standardStreamRedirect;      ///< Redirects standard streams to the terminal.

    std::mutex _eventMutex;                                      ///< Mutex for lazy event system creation.
    EventDataPtr _eventData;                                     ///< The data for the event system.
};

}
