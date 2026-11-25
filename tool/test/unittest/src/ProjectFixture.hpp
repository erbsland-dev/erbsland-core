// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NoConfigurationTool.hpp"
#include "SourceTestTool.hpp"
#include "TestToolApplication.hpp"

#include <erbsland/AnyStringBuilderStream.hpp>
#include <erbsland/Diagnostic.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/cterm/BoxFrameSource.hpp>
#include <tool/frame/ProjectFiles.hpp>
#include <tool/frame/ProjectRoot.hpp>
#include <tool/frame/ToolApplication.hpp>
#include <tool/idna/IdnaTool.hpp>
#include <tool/psl/PublicSuffixSource.hpp>
#include <tool/time_zone/TimeZoneSource.hpp>
#include <tool/unicode/UnicodeSource.hpp>

#include <exception>
#include <functional>
#include <utility>

using namespace el::text::literals;

/// Prepare an isolated marked project using public Core file APIs.
/// @notest{Unit-test fixture.}
class ProjectFixture : public el::UnitTest {
public: // implement UnitTest
    void setUp() override {
        _temporary = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        _root = _temporary->path() / "project"_el;
        _root.operations().createDirectoryOrThrow();
        _root = _root.resolveOrThrow();
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n"_el);
        (_root / "CMakeLists.txt"_el).content().writeTextOrThrow("project(erbsland-core LANGUAGES CXX)\n"_el);
    }

    void tearDown() override { _temporary.reset(); }

    auto additionalErrorMessages() -> std::string override {
        return el::StringConverter{el::join(_failureDiagnostic, _outputText, _errorText)}.toStdString();
    }

protected: // fixtures
    template <typename tTool = NoConfigurationTool>
    [[nodiscard]] auto runTool(
        const std::function<void(const el::conf::ValuePtr &)> &action = {},
        const std::function<void(TestToolApplication &)> &registration = {},
        const el::String &commandName = "probe"_el,
        const el::StringList &extraOptions = {},
        const bool selectProject = true) -> int {
        const auto output = el::stream::AnyStringBuilderStream::create();
        const auto error = el::stream::AnyStringBuilderStream::create();
        const auto redirect = el::stream::redirectStandardStreams(output, error);
        _failureDiagnostic = {};
        auto failure = std::exception_ptr{};
        auto previousAction = std::move(NoConfigurationTool::action);
        NoConfigurationTool::action = [&](const el::conf::ValuePtr &configuration) -> void {
            try {
                if (action) {
                    action(configuration);
                }
            } catch (const el::RuntimeError &error) {
                _failureDiagnostic = error.diagnostic()->toString();
                failure = std::current_exception();
            } catch (...) {
                failure = std::current_exception();
            }
        };
        // Convert only at Application's native argv boundary.
        auto project = el::StringConverter{_root.toString()}.toStdString();
        char executable[] = "erbsland-core-tool";
        auto command = el::StringConverter{commandName}.toStdString();
        char projectOption[] = "--project";
        auto storage = std::vector<std::string>{};
        for (const auto &option : extraOptions) {
            storage.emplace_back(el::StringConverter{option}.toStdString());
        }
        auto arguments = std::vector<char *>{executable, command.data()};
        if (selectProject) {
            arguments.push_back(projectOption);
            arguments.push_back(project.data());
        }
        for (auto &option : storage) {
            arguments.emplace_back(option.data());
        }
        const auto count = static_cast<int>(arguments.size());
        arguments.emplace_back(nullptr);
        auto application = TestToolApplication{count, arguments.data()};
        application.registerTool<tTool>();
        application.registerTool<tool::idna::IdnaTool>();
        application.registerSource<tool::unicode::UnicodeSource>();
        application.registerSource<tool::cterm::BoxFrameSource>();
        application.registerSource<tool::psl::PublicSuffixSource>();
        application.registerSource<tool::time_zone::TimeZoneSource>();
        if (registration) {
            registration(application);
        }
        auto result = 0;
        try {
            result = application.run();
        } catch (...) {
            NoConfigurationTool::action = std::move(previousAction);
            throw;
        }
        application.log().shutdown();
        _outputText = output->toString();
        _errorText = error->toString();
        NoConfigurationTool::action = std::move(previousAction);
        if (failure) {
            std::rethrow_exception(failure);
        }
        return result;
    }

    void runInsideApplication(const std::function<void()> &action) {
        REQUIRE_EQUAL(runTool([&](const auto &) -> void { action(); }), el::ExitCode::success().toRawValue());
    }

    /// Execute fixture assertions with exactly the declared shared sources initialized.
    /// @param identifiers Source dependencies of the fixture tool.
    /// @param action Assertions to run within the active application.
    void runWithSources(const el::StringList &identifiers, const std::function<void()> &action) {
        const auto previous = SourceTestTool::identifiers;
        SourceTestTool::identifiers = identifiers;
        try {
            REQUIRE_EQUAL(
                runTool<SourceTestTool>([&](const auto &) -> void { action(); }), el::ExitCode::success().toRawValue());
        } catch (...) {
            SourceTestTool::identifiers = previous;
            throw;
        }
        SourceTestTool::identifiers = previous;
    }

    [[nodiscard]] auto files() const -> tool::frame::ProjectFiles {
        return tool::frame::ProjectFiles{
            tool::frame::ProjectRoot{_root, TestToolApplication::coreProject()}.validate()};
    }

    void write(const el::String &relative, const el::String &text) {
        (_root / relative)
            .content()
            .writeTextOrThrow(
                text,
                el::PathWriteTextOptions{}.setCreateParents(true).setCreationMode(
                    el::PathCreateMode::CreateOrOverwrite));
    }

protected:
    el::String _outputText;          ///< Captured application and terminal output.
    el::String _errorText;           ///< Captured application error output.
    el::String _failureDiagnostic;   ///< Captured details of a failure inside the application.
    el::TempDirectoryPtr _temporary; ///< Fixture lifetime.
    el::Path _root;                  ///< Isolated marked repository.
};
