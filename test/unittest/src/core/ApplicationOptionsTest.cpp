// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/options/OptionModule.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionValue.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using el::core::Application;
using el::options::OptionModule;
using el::options::OptionModulePtr;
using el::options::OptionType;
using el::options::OptionValuesPtr;
using el::unit::ArgumentCount;
using el::unit::ArgumentIndex;
using el::unit::ElementIndex;
using el::unit::ExitCode;
using namespace el::text::literals;

TESTED_TARGETS(Application OptionModule OptionValue OptionValues Options)
class ApplicationOptionsTest final : public el::UnitTest {
public:
    void testApplicationStub() {
        char arg0[] = "tool";
        char arg1[] = "--verbose";
        char *argv[] = {arg0, arg1};

        auto scope = ApplicationTestScope<Application>{2, argv};
        auto &application = scope.app();
        REQUIRE(application.options() != nullptr);
        REQUIRE_EQUAL(application.commandLineArguments().count().toSizeT(), 2U);
        REQUIRE(application.commandLineArguments().get(ElementIndex{0}) == "tool"_el);
        REQUIRE(application.commandLineArguments().get(ElementIndex{1}) == "--verbose"_el);

        application.info().setApplicationName("Tool"_el);
        REQUIRE(application.info().applicationName() == "Tool"_el);

        application.setMainFn([]() -> ExitCode { return ExitCode{17}; });
        REQUIRE(application.run() != 0);
        REQUIRE(application.optionValues() == nullptr);
    }

    void testApplicationModuleMain() {
        char arg0[] = "tool";
        char arg1[] = "run";
        char arg2[] = "--name";
        char arg3[] = "Ada";
        char arg4[] = "-vv";
        char *argv[] = {arg0, arg1, arg2, arg3, arg4};

        auto scope = ApplicationTestScope<Application>{5, argv};
        auto &application = scope.app();
        auto moduleMainCalled = false;
        auto applicationMainCalled = false;
        auto moduleValues = OptionValuesPtr{};
        auto moduleMainGotFlagCount = false;
        auto module = OptionModule::create("run"_el);
        module->addOption("--name"_el).setType(OptionType::Text);
        module->addOption({"-v"_el, "--verbose"_el}).setType(OptionType::Flag);
        module->setMainFn(
            [&moduleMainCalled, &moduleValues, &moduleMainGotFlagCount](OptionValuesPtr values) -> ExitCode {
                moduleMainCalled = true;
                moduleValues = values;
                moduleMainGotFlagCount = values->getFlagCount("--verbose"_el) == ArgumentCount{2U};
                return ExitCode{23};
            });
        application.options()->addModule(module);
        application.setMainFn([&applicationMainCalled]() -> ExitCode {
            applicationMainCalled = true;
            return ExitCode{17};
        });

        REQUIRE_EQUAL(application.run(), 23);
        REQUIRE(application.optionValues() == moduleValues);
        REQUIRE(application.optionValues()->module() == module);
        REQUIRE(moduleValues->getText("--name"_el) == "Ada"_el);
        REQUIRE(moduleValues->value("--name"_el)->argumentIndex() == ArgumentIndex{3U});
        REQUIRE(moduleMainGotFlagCount);
        REQUIRE(moduleMainCalled);
        REQUIRE_FALSE(applicationMainCalled);
    }

    void testApplicationMainFallback() {
        char arg0[] = "tool";
        char *argv[] = {arg0};

        auto scope = ApplicationTestScope<Application>{1, argv};
        auto &application = scope.app();
        auto applicationMainCalled = false;
        application.setMainFn([&applicationMainCalled]() -> ExitCode {
            applicationMainCalled = true;
            return ExitCode{17};
        });

        REQUIRE_EQUAL(application.run(), 17);
        REQUIRE(application.optionValues() != nullptr);
        REQUIRE(applicationMainCalled);
    }

    void testApplicationSkipsModuleMainForNonSuccess() {
        char helpArg0[] = "tool";
        char helpArg1[] = "run";
        char helpArg2[] = "--help";
        char *helpArgv[] = {helpArg0, helpArg1, helpArg2};

        auto moduleMainCalled = false;
        auto applicationMainCalled = false;
        runHelpCase(helpArgv, moduleMainCalled, applicationMainCalled);

        char errorArg0[] = "tool";
        char errorArg1[] = "run";
        char errorArg2[] = "--name";
        char *errorArgv[] = {errorArg0, errorArg1, errorArg2};

        moduleMainCalled = false;
        applicationMainCalled = false;
        runErrorCase(errorArgv, moduleMainCalled, applicationMainCalled);
    }

private:
    void configureSkippedMainTest(
        Application &application, const OptionModulePtr &module, bool &moduleMainCalled, bool &applicationMainCalled) {
        module->setMainFn([&moduleMainCalled](OptionValuesPtr) -> ExitCode {
            moduleMainCalled = true;
            return ExitCode{23};
        });
        application.options()->addModule(module);
        application.setMainFn([&applicationMainCalled]() -> ExitCode {
            applicationMainCalled = true;
            return ExitCode{17};
        });
    }

    void runHelpCase(char *argv[], bool &moduleMainCalled, bool &applicationMainCalled) {
        auto scope = ApplicationTestScope<Application>{3, argv};
        auto &application = scope.app();
        auto module = OptionModule::create("run"_el);
        configureSkippedMainTest(application, module, moduleMainCalled, applicationMainCalled);

        REQUIRE_EQUAL(application.run(), 0);
        REQUIRE(application.optionValues() == nullptr);
        REQUIRE_FALSE(moduleMainCalled);
        REQUIRE_FALSE(applicationMainCalled);
    }

    void runErrorCase(char *argv[], bool &moduleMainCalled, bool &applicationMainCalled) {
        auto scope = ApplicationTestScope<Application>{3, argv};
        auto &application = scope.app();
        auto module = OptionModule::create("run"_el);
        module->addOption("--name"_el).setType(OptionType::Text);
        configureSkippedMainTest(application, module, moduleMainCalled, applicationMainCalled);

        REQUIRE(application.run() != 0);
        REQUIRE(application.optionValues() == nullptr);
        REQUIRE_FALSE(moduleMainCalled);
        REQUIRE_FALSE(applicationMainCalled);
    }
};
