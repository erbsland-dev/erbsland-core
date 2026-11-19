// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ToolApplication.hpp"

#include "ProjectRoot.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/DateTime.hpp>
#include <erbsland/FileLogWriterOptions.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogFileMode.hpp>
#include <erbsland/LogLineFormat.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogWriter.hpp>
#include <erbsland/LogWriterFilter.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/render/ResourceLoader.hpp>
#include <erbsland/Version.hpp>
#include <erbsland/Year.hpp>

#include <utility>

namespace tool::frame {

using namespace el::text::literals;

auto ToolApplication::files() const -> ProjectFiles & {
    if (!_files) {
        throw el::LogicFailure{"This command has no project file service."_el};
    }
    return *_files;
}

auto ToolApplication::createRenderEnvironment(
    const TemplateLanguage language, const el::resource::ResourcesConstPtr &resources) const
    -> el::render::EnvironmentPtr {
    if (language == TemplateLanguage::Cpp || language == TemplateLanguage::Text) {
        auto options = el::render::EnvironmentOptions{};
        if (language == TemplateLanguage::Cpp) {
            options.setExpressionDelimiters(el::render::Delimiters{"/*{"_el, "}*/"_el});
            options.setStatementDelimiters(el::render::Delimiters{"/*%"_el, "%*/"_el});
        }
        options.setAutomaticEscapingEnabled(false);
        auto renderer = el::render::Environment::create(options);
        renderer->addLayoutLoader(el::render::ResourceLoader::create(resources, "erbsland-core-tool"_el));
        auto context = el::render::Context{};
        context.set("current_year"_el, _currentYear);
        context.set(cCopyrightLineKey, _copyrightLine);
        context.set(cLicenseLineKey, _licenseLine);
        renderer->setGlobalContext(std::move(context));
        return renderer;
    }
    throwTemplateLanguageFailure();
}

void ToolApplication::initialize() {
    _currentYear = el::time::DateTime::now().year().toRawValue();
    // Enable interactive terminal output.
    enableTerminal();
    // Setup basic information for the application.
    info().setApplicationName("erbsland-core-tool"_el);
    info().setAuthorName("Tobias Erbsland"_el);
    info().setCopyrightLine("Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev"_el);
    info().setApplicationVersion(el::Version{1, 0, 0});
}

void ToolApplication::registerCommandLineOptions(const el::OptionsPtr &options) {
    _registrationClosed = true;
    options->setHelpDescription("Erbsland Core repository development tools."_el);
    options->addOption({"--project"_el, cProjectOption})
        .setType(el::OptionType::Text)
        .setHelpTitle("Use this Erbsland Core project directory."_el);
    options->addOption({"--config"_el, cConfigOption})
        .setType(el::OptionType::Text)
        .setHelpTitle("Read a command configuration (project-confined for repository commands)."_el);
    options->addOption({"-v"_el, "--verbose"_el, cVerboseOption})
        .setType(el::OptionType::Flag)
        .setHelpTitle("Enable verbose output for the tool."_el);
    options->addOption({"--log-file"_el, cLogFileOption})
        .setType(el::OptionType::Text)
        .setHelpTitle("Append verbose log output to the given file."_el);
    _commandTools.clear();
    for (const auto &tool : _tools) {
        auto module = tool->optionModule();
        if (module == nullptr) {
            throwRegistrationFailure(
                el::StringFormat{"Tool \"{}\": No module specified."_el}.build(tool->identifier()));
        }
        if (!module->hasName(tool->identifier())) {
            throwRegistrationFailure(
                el::StringFormat{"Tool \"{}\": Module name does not match tool identifier."_el}.build(
                    tool->identifier()));
        }
        if (module->help().title().isEmpty()) {
            throwRegistrationFailure(
                el::StringFormat{"Tool \"{}\": Module does not specify a title."_el}.build(tool->identifier()));
        }
        for (const auto &registered : _commandTools) {
            if (registered.first->name() == module->name()) {
                throwRegistrationFailure("The canonical command name is already registered."_el);
            }
        }
        _commandTools.emplace_back(module, tool);
        options->addModule(module);
    }
}

auto ToolApplication::main() -> el::ExitCode {
    const auto values = optionValues();
    const auto tool = getToolOrThrow(values->moduleName());
    initializeCommandServices(tool->requiresProject());

    const auto log = logStream();
    if (auto *task = dynamic_cast<PreCommitTask *>(tool.get())) {
        task->_taskContext = PreCommitContext{log, {}};
    }
    prepareTool(*tool, values, log, _configuration);
    tool->execute();
    return tool->exitCode();
}

void ToolApplication::initializeCommandServices(const bool requiresProject) {
    if (!requiresProject) {
        if (const auto projectOption = optionValues()->value(cProjectOption)) {
            throw el::OptionError{
                "Invalid Project Option"_el, "--project does not apply to this command."_el, projectOption};
        }
    }
    initializeLogging();
    if (requiresProject) {
        initializeProjectFiles();
        _configuration = readConfigurationOrThrow();
        configureRendering(_configuration);
    }
}

void ToolApplication::validateToolMeta(const ToolPtr &tool) {
    validateIdentifier(tool->identifier());
    if (tool->title().isEmpty() || tool->title().characterLength() > el::CpCount{60}) {
        throwRegistrationFailure(el::StringFormat{"Tool \"{}\": Title is invalid."_el}.build(tool->identifier()));
    }
    if (tool->description().isEmpty()) {
        throwRegistrationFailure(
            el::StringFormat{"Tool \"{}\": Description is required."_el}.build(tool->identifier()));
    }
}

void ToolApplication::initializeLogging() {
    // Set a filter for the console to only show trace messages if verbose mode is enabled.
    el::LogWriterFilter consoleFilter;
    if (optionValues()->getFlag(cVerboseOption)) {
        consoleFilter.setAcceptedLevels(el::LogLevel::All);
    } else {
        consoleFilter.setAcceptedLevels({el::LogLevel::Information, el::LogLevel::Warning, el::LogLevel::Error});
    }
    // If --log-file is specified, create a writer for appending to that file.
    if (const auto logOption = optionValues()->value(cLogFileOption)) {
        auto path = el::Path{logOption->getText()};
        try {
            if (path.isEmpty()) {
                throw el::OptionError{"Invalid Log File"_el, "The log file path must not be empty."_el, logOption};
            }
            path = path.toAbsoluteOrThrow();
            if (!path.parent().info().isDirectory() || (path.info().exists() && !path.info().isRegularFile())) {
                throw el::OptionError{
                    "Invalid Log File"_el, "Select a regular file in an existing directory."_el, logOption};
            }
        } catch (const el::PathError &) {
            throw el::OptionError{
                "Invalid Log File"_el,
                "The log file path could not be resolved."_el,
                logOption,
                std::current_exception()};
        }
        _logFilePath = path;
    }
    log().setConfiguration(loggingConfiguration(el::LogWriter::createForConsole(terminal()), consoleFilter));
}

auto ToolApplication::loggingConfiguration(el::LogWriterPtr writer, el::LogWriterFilter filter) const
    -> el::LogConfiguration {
    auto configuration = el::LogConfiguration{};
    auto format = el::LogLineFormat{};
    format.setPattern("{message}"_el);
    configuration.addWriter(std::move(writer), std::move(filter), format);
    if (!_logFilePath.isEmpty()) {
        auto options = el::FileLogWriterOptions{_logFilePath}.setMode(el::LogFileMode::Append);
        format.setPattern("{time} {name} {message}"_el);
        configuration.addWriter(el::LogWriter::createForFile(options), {}, format);
    }
    return configuration;
}

void ToolApplication::initializeProjectFiles() {
    el::Path projectRoot;
    if (const auto projectOption = optionValues()->value(cProjectOption)) {
        try {
            projectRoot = ProjectRoot{el::Path{projectOption->getText()}}.validate();
        } catch (const el::ApplicationError &error) {
            throw el::OptionError{
                "Invalid Project Directory"_el,
                error.description(),
                optionValues()->value(cProjectOption),
                std::current_exception()};
        }
    } else {
        projectRoot = ProjectRoot{el::Path::currentDirectory()}.discover();
    }
    _files = std::make_unique<ProjectFiles>(projectRoot);
}

auto ToolApplication::readConfigurationOrThrow() -> el::conf::DocumentPtr {
    el::Path configPath;
    if (const auto configOption = optionValues()->value(cConfigOption)) {
        configPath = _files->checked(configOption, el::PathType::RegularFile);
    } else {
        configPath = _files->projectRoot() / el::String{cDefaultConfigPath};
    }
    configPath = configPath.toAbsoluteOrThrow();
    return _files->readConfiguration(
        configPath, true, el::StringMap<el::String>{{"year"_el, el::StringFormat{"{}"_el}.build(_currentYear)}});
}

auto ToolApplication::renderingRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;

    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Generated notices"_el}, Description{"Copyright and license notices shared by generated files."_el});
    rules.addRule(
        cCopyrightLineKey,
        RuleType::Text,
        Title{"Generated Copyright Notice"_el},
        Description{"The single-line notice inserted into generated files, without a comment prefix. "
                    "The default uses the UTC year captured at application startup."_el},
        Default{el::StringFormat{"Copyright (c) {} Tobias Erbsland - https://erbsland.dev"_el}.build(_currentYear)},
        Matches{R"(\A[^\r\n]+\z)"_el});
    rules.addRule(
        cLicenseLineKey,
        RuleType::Text,
        Title{"Generated License Notice"_el},
        Description{
            "The nonempty, single-line license notice inserted into generated files, without a comment prefix."_el},
        Default{"SPDX-License-Identifier: Apache-2.0"_el},
        Matches{R"(\A[^\r\n]+\z)"_el});
    return rules.takeRules();
}

void ToolApplication::configureRendering(const el::conf::DocumentPtr &configuration) {
    const auto section = configuration->value("rendering"_el);
    if (!section) {
        throwConfigurationError(configuration, "The configuration requires a [rendering] section."_el);
    }
    renderingRules()->validate(section, 1);
    _copyrightLine = section->getTextOrThrow(cCopyrightLineKey);
    _licenseLine = section->getTextOrThrow(cLicenseLineKey);
}

auto ToolApplication::getToolOrThrow(const el::String &identifier) -> ToolPtr {
    for (const auto &[module, tool] : _commandTools) {
        if (module->name() == identifier) {
            return tool;
        }
    }
    throwRegistrationFailure(el::StringFormat{"Tool \"{}\" not found after options validation."_el}.build(identifier));
}

void ToolApplication::throwRegistrationFailure(el::String description) {
    throw el::LogicFailure{el::StringFormat{"Invalid tool or source registration. {}"_el}.build(description)};
}

void ToolApplication::throwConfigurationError(const el::conf::ValuePtr &value, el::String description) {
    throw el::conf::ConfError{value, "Invalid Tool Configuration"_el, std::move(description)};
}

void ToolApplication::throwTemplateLanguageFailure() {
    throw el::LogicFailure{
        "Failed to create template environment. The requested template language is not supported by this tool."_el};
}

auto app() noexcept -> ToolApplication & {
    return dynamic_cast<ToolApplication &>(el::application());
}

}
