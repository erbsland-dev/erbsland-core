// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PreCommitTask.hpp"
#include "ProjectFiles.hpp"
#include "TemplateLanguage.hpp"
#include "Tool.hpp"
#include "ToolProject.hpp"

#include "impl/DataSourceRegistration.hpp"
#include "impl/DataSourceTraits.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/StringList.hpp>

#include <concepts>
#include <memory>
#include <typeinfo>
#include <utility>
#include <vector>

namespace tool::frame {

using namespace el::text::literals;

/// The modular repository application, using Core's command-line parser and lifecycle.
/// @tested{ToolApplicationTest DataSourceTest}
class ToolApplication : public el::Application {
    /// Option key for --project, selecting the repository root.
    constexpr static auto cProjectOption = "project"_el;
    /// Option key for --config, selecting an explicit configuration file.
    constexpr static auto cConfigOption = "config"_el;
    /// Option key for --verbose, enabling detailed logging.
    constexpr static auto cVerboseOption = "verbose"_el;
    /// Option key for --log-file, selecting an additional logging destination.
    constexpr static auto cLogFileOption = "log-file"_el;
    /// Rendering configuration key for the generated-file copyright notice.
    constexpr static auto cCopyrightLineKey = "copyright_line"_el;
    /// Rendering configuration key for the generated-file license notice.
    constexpr static auto cLicenseLineKey = "license_line"_el;

public:
    /// Create a registration-only application without initializing native arguments.
    /// @param project Repository identity and default configuration.
    explicit ToolApplication(ToolProject project);
    /// Bind the command line and repository identity for this executable.
    /// @param argc Native argument count.
    /// @param argv Native argument vector.
    /// @param project Repository identity and default configuration.
    ToolApplication(int argc, char *argv[], ToolProject project);

public: // accessors
    /// Get the repository identity supplied by the executable.
    [[nodiscard]] auto project() const noexcept -> const ToolProject & { return _project; }

public: // registration
    /// Register a built-in tool before running the application.
    /// @tparam tTool Concrete Tool type with a default constructor.
    template <std::derived_from<Tool> tTool>
    void registerTool() {
        requireOpenRegistration();
        // Core option modules use shared ownership; keep this conversion at registration.
        auto tool = std::make_shared<tTool>();
        validateToolMeta(tool);
        requireAvailableIdentifier(tool->identifier());
        _tools.append(tool);
    }
    /// Register one task, also registering its command when it implements Tool.
    /// @tparam T Default-constructible task implementation.
    template <std::derived_from<PreCommitTask> T>
    void registerPreCommitTask() {
        registerTask(std::make_shared<T>());
    }
    /// Register a default-constructible source before command-line processing.
    /// @tparam T Concrete DataSource type providing static sourceIdentifier() returning String.
    template <impl::DataSourceType T>
    void registerSource() {
        requireOpenRegistration();
        registerDataSource(T::sourceIdentifier(), std::make_unique<T>());
    }

public: // shared API
    /// Get tasks in their explicit registration order.
    [[nodiscard]] auto preCommitTasks() const -> const el::List<PreCommitTaskPtr> & { return _tasks; }
    /// Assign task services and prepare a dual-interface command using its default options.
    /// @param task Task to prepare before execute().
    /// @param context Services retained for the invocation.
    /// @throws el::RuntimeError If task options, configuration, or preparation fail.
    void preparePreCommitTask(PreCommitTask &task, PreCommitContext context);
    /// Create independent logging routes using the validated application log-file option.
    /// @param writer Console or capture writer; the caller controls its visibility policy.
    /// @param filter Levels routed to that writer.
    /// @return Configuration with optional detailed file logging.
    [[nodiscard]] auto loggingConfiguration(el::LogWriterPtr writer, el::LogWriterFilter filter = {}) const
        -> el::LogConfiguration;
    /// Access files and paths.
    [[nodiscard]] auto files() const -> ProjectFiles &;
    /// Access an already initialized source without triggering loading.
    /// @tparam T Concrete DataSource type providing static sourceIdentifier() returning String.
    /// @return The application-owned source instance.
    template <impl::DataSourceType T>
    [[nodiscard]] auto source() -> T & {
        auto &registered = initializedSource(T::sourceIdentifier());
        if (typeid(registered) != typeid(T)) {
            throwSourceAccessFailure(T::sourceIdentifier(), "The registered source has a different concrete type."_el);
        }
        return static_cast<T &>(registered);
    }
    /// Create a render environment.
    /// @param language The template language to use for rendering.
    /// @param resources Optional explicit resource provider; otherwise use the application resources.
    /// @return A compiled-resource render environment without filesystem loaders.
    [[nodiscard]] auto createRenderEnvironment(
        TemplateLanguage language, const el::resource::ResourcesConstPtr &resources = {}) const
        -> el::render::EnvironmentPtr;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    [[nodiscard]] auto main() -> el::ExitCode override;

private: // source and command registration
    /// Register and validate one task instance, sharing ownership with the command registry when applicable.
    void registerTask(PreCommitTaskPtr task);
    /// Run the common command preparation lifecycle with explicitly selected options and logging.
    /// @param tool The command to prepare.
    /// @param values Parsed command-line options.
    /// @param log Stream for this invocation.
    /// @param configuration The repository configuration, empty for standalone commands.
    void prepareTool(
        Tool &tool,
        const el::OptionValuesPtr &values,
        const el::LogStreamPtr &log,
        const el::conf::DocumentPtr &configuration);
    /// Parse a registered command with no command-specific arguments, applying declared defaults.
    [[nodiscard]] auto defaultToolOptions(const Tool &tool) const -> el::OptionValuesPtr;
    /// Require registration to precede command-line processing.
    void requireOpenRegistration() const;
    /// Require an identifier to be unoccupied by tools, sources, and reserved sections.
    /// @param identifier The proposed tool or source identifier.
    void requireAvailableIdentifier(const el::String &identifier) const;
    /// Validate the shared ASCII identifier syntax.
    /// @param identifier The proposed tool or source identifier.
    static void validateIdentifier(const el::String &identifier);
    /// Validate identity and take ownership of a newly constructed source.
    /// @param identifier The concrete source's static identifier.
    /// @param source The instance to register without configuring or loading it.
    void registerDataSource(el::String identifier, std::unique_ptr<DataSource> source);
    /// Locate a registration without triggering configuration or loading.
    /// @param identifier The static source identifier to resolve.
    /// @return The application's registration entry.
    [[nodiscard]] auto sourceRegistration(const el::String &identifier) -> impl::DataSourceRegistration &;
    /// Locate a source whose initialization completed successfully.
    /// @param identifier The static source identifier to resolve.
    /// @return The initialized polymorphic source instance.
    [[nodiscard]] auto initializedSource(const el::String &identifier) -> DataSource &;
    /// Report a programming error in source dependency declaration or typed access.
    /// @param identifier The source involved in the error.
    /// @param description The reason access failed.
    [[noreturn]] static void throwSourceAccessFailure(const el::String &identifier, const el::String &description);
    /// Apply common section requirements and validation defaults for a tool or source.
    /// @param configuration The parsed application configuration.
    /// @param identifier The required section name.
    /// @param rules Rules requiring a regular section, or null to prohibit that section.
    /// @return The validated section or null for a configuration-free component.
    /// @throws el::conf::ConfError If the section is missing, prohibited, or invalid.
    [[nodiscard]] static auto validateSection(
        const el::conf::DocumentPtr &configuration, const el::String &identifier, const el::conf::vr::RulesPtr &rules)
        -> el::conf::ValuePtr;
    /// Validate all source sections without configuring or loading a source.
    /// @param configuration Parsed application configuration.
    /// @param identifiers Required sources in dependency order; duplicates are ignored.
    /// @return Deduplicated source identifiers in dependency order, with configurations retained in their
    /// registrations.
    /// @throws el::conf::ConfError If any selected source section is invalid.
    [[nodiscard]] auto validateSources(const el::conf::DocumentPtr &configuration, const el::StringList &identifiers)
        -> el::StringList;
    /// Configure and initialize new sources, rebinding all selected sources to the current log.
    /// @param identifiers Validated source identifiers in dependency order, without duplicates.
    /// @param log Stream for this command or task invocation.
    /// @throws el::ApplicationError If source configuration or loading fails.
    void initializeSources(const el::StringList &identifiers, const el::LogStreamPtr &log);
    /// Report invalid built-in metadata or registration.
    /// @param description The reason registration failed.
    [[noreturn]] static void throwRegistrationFailure(el::String description);
    /// Report invalid configuration while retaining its source location.
    /// @param value The value responsible for the failure.
    /// @param description The reason configuration is invalid.
    [[noreturn]] static void throwConfigurationError(const el::conf::ValuePtr &value, el::String description);
    /// Report an unsupported template language.
    [[noreturn]] static void throwTemplateLanguageFailure();
    /// Validate if the tools meta data is valid.
    /// Checks the identifier, title and description.
    /// @param tool Shared pointer to the tool module to validate.
    static void validateToolMeta(const ToolPtr &tool);
    /// Validate project selection and start the services needed by the selected command.
    /// @param requiresProject Whether to initialize repository services in addition to logging.
    void initializeCommandServices(bool requiresProject);
    /// Initialize the logging system.
    void initializeLogging();
    /// Build the rendering schema with resolved defaults and header constraints.
    /// @return The shared rendering validation rules.
    [[nodiscard]] auto renderingRules() const -> el::conf::vr::RulesPtr;
    /// Validate shared rendering settings and let VR assign omitted defaults.
    /// @param configuration The parsed application configuration.
    /// @throws el::conf::ConfError If the rendering section is missing or invalid.
    void configureRendering(const el::conf::DocumentPtr &configuration);
    /// Scan for the project root and create the `ProjectFiles` instance.
    void initializeProjectFiles();
    /// Read the configuration file.
    /// @return The configuration document with application placeholders expanded.
    /// @throws el::conf::ConfError If confined reading, parsing, or expansion fails.
    auto readConfigurationOrThrow() -> el::conf::DocumentPtr;
    /// Get the tool associated with the selected canonical command name.
    /// @param identifier The canonical module name selected by the option parser.
    /// @return The tool instance.
    auto getToolOrThrow(const el::String &identifier) -> ToolPtr;

private:
    ToolProject _project;                 ///< Identity used throughout this invocation.
    el::List<PreCommitTaskPtr> _tasks;    ///< Ordered pre-commit participants.
    el::conf::DocumentPtr _configuration; ///< Shared parsed configuration, validated per task.
    el::Path _logFilePath;                ///< Validated optional log-file destination.
    bool _registrationClosed = false;     ///< Registration closes before options are constructed.
    // Core's copy-on-write List requires copyable entries; this registry uniquely owns its sources.
    std::vector<impl::DataSourceRegistration> _sources;                 ///< Source ownership, identity, and readiness.
    int _currentYear{};                                                 ///< UTC year captured once during startup.
    el::String _copyrightLine;                                          ///< Expanded generated-file copyright notice.
    el::String _licenseLine;                                            ///< Expanded generated-file license notice.
    std::vector<std::pair<el::OptionModulePtr, ToolPtr>> _commandTools; ///< Canonical command modules and their tools.
    el::List<std::shared_ptr<Tool>> _tools; ///< Registered Core modules and their shared lifetime.
    std::unique_ptr<ProjectFiles> _files;   ///< The files to operate on.
};

/// Access the current application instance.
/// @return The active maintenance application.
/// @tested{ToolApplicationTest DataSourceTest}
[[nodiscard]] auto app() noexcept -> ToolApplication &;

}
