// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BuildDocTool.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/all_conf_vr.hpp>
#include <erbsland/all_conf_vr_builder.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/Definitions.hpp>
#include <erbsland/ElapsedTimer.hpp>
#include <erbsland/EnvironmentChanges.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/ProcessInfo.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/System.hpp>

#include <chrono>
#include <thread>

namespace tool::doc {

using namespace el::text::literals;

auto BuildDocTool::identifier() const -> el::String {
    return "doc_build"_el;
}

auto BuildDocTool::title() const -> el::String {
    return "Build Sphinx documentation incrementally."_el;
}

auto BuildDocTool::description() const -> el::String {
    return "Build documentation using cached Doxygen XML and Sphinx output, or discard caches with --full-rebuild."_el;
}

auto BuildDocTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"--full-rebuild"_el, "--force"_el, cFullRebuildOption})
        .setType(el::OptionType::Flag)
        .setHelpTitle("Regenerate all documentation caches."_el);
    module->addOption({"--no-filter"_el, cNoFilterOption})
        .setType(el::OptionType::Flag)
        .setHelpTitle("Display complete Sphinx and Doxygen output."_el);
    module->addOption({"--show-suppressed"_el, cShowSuppressedOption})
        .setType(el::OptionType::Flag)
        .setHelpTitle("Display configured accepted Doxygen warnings."_el);
    return module;
}

auto BuildDocTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Documentation build"_el},
        Description{"Build execution and diagnostic filtering for Sphinx and Doxygen."_el});
    rules.addRule(
        cIgnoredWarningsConfig,
        RuleType::SectionList,
        IsOptional{},
        Title{"Accepted Doxygen warnings"_el},
        Description{"Narrow source and message exceptions."_el});
    rules.addRule(
        "ignored_doxygen_warnings.vr_entry"_el,
        RuleType::Section,
        Title{"Accepted Doxygen warning"_el},
        Description{"Source and message filters for one documented warning exception."_el});
    auto entry = rules.createBranch("ignored_doxygen_warnings.vr_entry"_el);
    entry.addRule(
        cSourceGlobConfig,
        RuleType::Text,
        Minimum{1},
        Title{"Warning source glob"_el},
        Description{"Path glob matching source files whose warnings may be suppressed."_el});
    entry.addRule(
        cMessageContainsConfig,
        RuleType::Text,
        Default{""_el},
        Title{"Warning message text"_el},
        Description{"Literal substring required in an accepted warning; empty matches all messages."_el});
    entry.addRule(
        cMessageRegexConfig,
        RuleType::Text,
        Default{""_el},
        Title{"Warning message expression"_el},
        Description{"Optional regular expression required in an accepted warning."_el});
    entry.addRule(
        cReasonConfig,
        RuleType::Text,
        Minimum{1},
        Title{"Acceptance reason"_el},
        Description{"Explanation of why matching warnings are accepted."_el});
    return rules.takeRules();
}

void BuildDocTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    _root = frame::app().files().projectRoot();
    _fullRebuild = values->getFlag(cFullRebuildOption);
    _filterOutput = !values->getFlag(cNoFilterOption);
    _filter = std::make_unique<DocumentationOutputFilter>(_root, values->getFlag(cShowSuppressedOption));
    if (const auto entries = configuration->getSectionList(cIgnoredWarningsConfig)) {
        for (const auto &entry : *entries) {
            try {
                _filter->addRule(
                    entry->getTextOrThrow(cSourceGlobConfig),
                    entry->getTextOrThrow(cMessageContainsConfig),
                    entry->getTextOrThrow(cMessageRegexConfig),
                    entry->getTextOrThrow(cReasonConfig));
            } catch (const el::re::RegExError &) {
                throw el::conf::ConfError{
                    entry->valueOrThrow(cMessageRegexConfig),
                    "Invalid Warning Message Pattern"_el,
                    "The warning message filter must be a valid regular expression."_el,
                    std::current_exception()};
            } catch (const el::ParseError &) {
                throw el::conf::ConfError{
                    entry->valueOrThrow(cSourceGlobConfig),
                    "Invalid Warning Source Pattern"_el,
                    "The warning source filter must be a valid path glob."_el,
                    std::current_exception()};
            }
        }
    }
}

void BuildDocTool::removeGeneratedDirectory(const el::String &name) {
    const auto path = frame::app().files().checked(el::Path{name});
    auto info = path.info();
    if (!info.exists()) {
        log()->trace("Skipping missing "_el, name);
        return;
    }
    if (!info.isDirectory()) {
        throw el::ApplicationError{
            "Documentation Cache Removal Failed"_el, "Refusing to remove a non-directory cache path."_el}
            .withSourcePath(path.toString());
    }
    log()->trace("Removing "_el, name);
    for (auto attempt = 0; attempt < 3; ++attempt) {
        try {
            path.operations().removeOrThrow(el::PathRemoveOptions{}.setRecursive(true));
            return;
        } catch (const el::PathError &) {
            if (attempt == 2) {
                throw;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{200});
        }
    }
}

auto BuildDocTool::sphinxExecutable() const -> el::Path {
#ifdef EL_OS_WINDOWS
    const auto relative = el::Path{".venv/Scripts/sphinx-build.exe"_el};
#else
    const auto relative = el::Path{".venv/bin/sphinx-build"_el};
#endif
    const auto executable = frame::app().files().checked(relative);
    const auto info = executable.info();
    if (!info.isRegularFile() || !info.isExecutable() || info.fileSize() > el::ByteCount{1024U * 1024U}) {
        throw el::ApplicationError{
            "Sphinx Executable Unavailable"_el,
            "Could not find a safe executable sphinx-build in the project virtual environment."_el}
            .withSourcePath(executable.toString());
    }
    return executable;
}

auto BuildDocTool::sphinxOptions() const -> el::SubprocessOptions {
    auto options = el::SubprocessOptions{}.setWorkingDirectory(_root).setInheritStandardInput(false);
    auto names = el::StringList{
        "HOME"_el,
        "LANG"_el,
        "LC_ALL"_el,
        "LC_CTYPE"_el,
        "PATH"_el,
        "TMPDIR"_el,
        "TEMP"_el,
        "TMP"_el,
    };
#ifdef EL_OS_WINDOWS
    const auto bin = _root / ".venv/Scripts"_el;
    // Windows Python and child tools require the system directory in a clean environment.
    names.append("SystemRoot"_el).append("WINDIR"_el);
#else
    const auto bin = _root / ".venv/bin"_el;
#endif
    options.setEnvironmentChanges(
        el::EnvironmentChanges{}
            .setInherit(std::move(names))
            .prependPath(bin)
            .setVariable("VIRTUAL_ENV"_el, (_root / ".venv"_el).toString())
            .setVariable("ERBSLAND_CORE_TOOL"_el, el::ProcessInfo{}.executablePathOrThrow().toNativeString()));
    return options;
}

void BuildDocTool::outputLine(const el::String &line) {
    _captured.append(line);
    if (const auto filtered = _filter->filterLine(line)) {
        log()->info(*filtered);
    }
}

void BuildDocTool::outputBlock(const el::String &block) {
    // Iterate raw slices so malformed bytes are preserved when joining callback boundaries.
    auto rest = block;
    while (true) {
        const auto parts = rest.splitAtFirst(el::CharSet{U'\n'});
        if (!parts) {
            _pendingLine.append(rest);
            return;
        }
        _pendingLine.append(parts->first);
        outputLine(el::String{_pendingLine}.trimmed(el::CharSet{"\r"_el}, el::StringSide::Back));
        _pendingLine.clear();
        rest = parts->second;
    }
}

auto BuildDocTool::runFiltered(const el::Path &executable, const el::StringList &arguments) -> el::ExitCode {
    auto loop = el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit));
    auto process = el::SubprocessPtr{};
    auto code = el::ExitCode::failure();
    auto failure = el::String{};
    auto truncated = false;
    loop->invoke([&]() -> void {
        auto options = sphinxOptions();
        options.setStandardOutputMode(el::SubprocessOutputMode::Capture)
            .setMergeStandardError(true)
            .setCaptureLimit(el::SubprocessOptions::cMaximumCaptureLimit);
        process = loop->get<el::System>().createSubprocess(executable, arguments, options);
        process->events()
            .onStandardOutput([&](const el::String &block) -> void { outputBlock(block); })
            .onOutputTruncated([&]() -> void { truncated = true; })
            .onExit([&](el::ExitCode result) -> void { code = result; })
            .onError([&](const el::PlatformErrorContext &context) -> void { failure = context.toString(); })
            .onFinal([&]() -> void { loop->stop(); });
        try {
            process->start();
        } catch (const el::PlatformError &) { // Error/final are delivered by the source.
        }
    });
    loop->run();
    if (loop->hasError()) {
        std::rethrow_exception(loop->takeError());
    }
    if (!_pendingLine.isEmpty()) {
        outputLine(_pendingLine);
        _pendingLine.clear();
    }
    if (truncated) {
        log()->warn("Subprocess output was truncated; failure replay is incomplete."_el);
    }
    if (code.isFailure()) {
        log()->info("Complete sphinx-build output after failure:"_el);
        for (const auto &line : _captured) {
            log()->info(_filter->shortenPaths(line));
        }
    }
    for (const auto &line : _filter->summary()) {
        log()->info(line);
    }
    if (!failure.isEmpty()) {
        throw el::ApplicationError{"Sphinx Execution Failed"_el, failure};
    }
    return code;
}

void BuildDocTool::execute() {
    if (_filterOutput) {
        log()->info("Filtered output (use --no-filter to show full tool output)."_el);
    }
    if (_fullRebuild) {
        removeGeneratedDirectory("_doxygen_input"_el);
        removeGeneratedDirectory("_build"_el);
    }
    const auto timer = el::ElapsedTimer{};
    const auto executable = sphinxExecutable();
    const auto arguments = el::StringList{"-j"_el, "1"_el, "doc"_el, "_build"_el};
    log()->trace("Running sphinx-build -j 1 doc _build"_el);
    auto code = el::ExitCode::success();
    if (_filterOutput) {
        code = runFiltered(executable, arguments);
    } else {
        auto options = sphinxOptions();
        options.setInheritStandardInput(true);
        auto process = el::Subprocess::create(executable, arguments, options);
        process->start();
        const auto status = process->wait();
        if (!status.hasExited()) {
            throw el::ApplicationError{"Documentation Build Failed"_el, "sphinx-build terminated abnormally."_el};
        }
        code = el::ExitCode{*status.exitCode()};
    }
    if (code.isFailure()) {
        throw el::ApplicationError{
            "Documentation Build Failed"_el,
            el::StringFormat{"sphinx-build failed with exit code {}."_el}.build(code.toRawValue())};
    }
    if (_filterOutput) {
        log()->info("sphinx-build completed successfully."_el);
    }
    log()->info(
        el::StringFormat{"Documentation build completed in {:number:notation=fixed,precision=1}s."_el}.build(
            timer.elapsed().toSecondsWithFractions()));
}

}
