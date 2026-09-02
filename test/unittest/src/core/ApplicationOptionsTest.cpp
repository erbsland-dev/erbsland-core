// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/options/OptionError.hpp>
#include <erbsland/options/OptionErrorContext.hpp>
#include <erbsland/options/OptionModule.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionValue.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/stream/AnyStringBuilderStream.hpp>
#include <erbsland/stream/StandardStreams.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

using el::core::Application;
using el::options::OptionError;
using el::options::OptionErrorContext;
using el::options::OptionModule;
using el::options::OptionModulePtr;
using el::options::OptionType;
using el::options::OptionValuePtr;
using el::options::OptionValuesPtr;
using el::unit::ArgumentCount;
using el::unit::ArgumentIndex;
using el::unit::ExitCode;
using el::unit::ItemIndex;
using namespace el::text::literals;

TESTED_TARGETS(Application OptionModule OptionValue OptionValues Options)
class ApplicationOptionsTest final : public el::UnitTest {
public:
    void testInitializeFunctionRunsBeforeMain() {
        auto scope = ApplicationTestScope<Application>{};
        auto &application = scope.app();
        auto initializeCount = 0;
        auto mainSawInitialization = false;
        application.setInitializeFn([&initializeCount]() -> void { ++initializeCount; });
        application.setMainFn([&initializeCount, &mainSawInitialization]() -> ExitCode {
            mainSawInitialization = initializeCount == 1;
            return ExitCode{17};
        });

        REQUIRE_EQUAL(application.run(), 17);
        REQUIRE_EQUAL(initializeCount, 1);
        REQUIRE(mainSawInitialization);
    }

    void testApplicationStub() {
        char arg0[] = "tool";
        char arg1[] = "--verbose";
        char *argv[] = {arg0, arg1};

        auto scope = ApplicationTestScope<Application>{2, argv};
        auto &application = scope.app();
        const auto streamRedirect = redirectStandardStreams();
        REQUIRE(application.options());
        REQUIRE_EQUAL(application.commandLineArguments().count().toSizeT(), 2U);
        REQUIRE_EQUAL(application.commandLineArguments().get(ItemIndex{0}), "tool"_el);
        REQUIRE_EQUAL(application.commandLineArguments().get(ItemIndex{1}), "--verbose"_el);

        application.info().setApplicationName("Tool"_el);
        REQUIRE_EQUAL(application.info().applicationName(), "Tool"_el);

        application.setMainFn([]() -> ExitCode { return ExitCode{17}; });
        REQUIRE_NOT_EQUAL(application.run(), 0);
        REQUIRE_FALSE(application.optionValues());
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
        REQUIRE_EQUAL(application.optionValues(), moduleValues);
        REQUIRE_EQUAL(application.optionValues()->module(), module);
        REQUIRE_EQUAL(moduleValues->getText("--name"_el), "Ada"_el);
        REQUIRE_EQUAL(moduleValues->value("--name"_el)->argumentIndex(), ArgumentIndex{3U});
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
        REQUIRE(application.optionValues());
        REQUIRE(applicationMainCalled);
    }

    void testApplicationSkipsModuleMainForNonSuccess() {
        const auto streamRedirect = redirectStandardStreams();
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

    void testApplicationRendersDetailedHelpAndModuleOverview() {
        {
            char arg0[] = "tool";
            char arg1[] = "--help=--name";
            char *argv[] = {arg0, arg1};
            const auto output = el::stream::AnyStringBuilderStream::create();
            const auto error = el::stream::AnyStringBuilderStream::create();
            const auto streamRedirect = el::stream::redirectStandardStreams(output, error);
            auto scope = ApplicationTestScope<Application>{2, argv};
            auto &application = scope.app();
            auto mainCalled = false;
            application.options()
                ->addOption("--name"_el)
                .setType(OptionType::Text)
                .setHelpTitle("Name Option"_el)
                .setHelpDescription("Select the displayed name."_el);
            application.setMainFn([&mainCalled]() -> ExitCode {
                mainCalled = true;
                return ExitCode::success();
            });

            REQUIRE_EQUAL(application.run(), 0);
            const auto text = el::text::StringConverter{output->toU8String()}.toStdString();
            REQUIRE_NOT_EQUAL(text.find("Name Option\n"), std::string::npos);
            REQUIRE_NOT_EQUAL(text.find("tool --name=<value>"), std::string::npos);
            REQUIRE_FALSE(application.optionValues());
            REQUIRE_FALSE(mainCalled);
        }

        {
            char arg0[] = "tool";
            char *argv[] = {arg0};
            const auto output = el::stream::AnyStringBuilderStream::create();
            const auto error = el::stream::AnyStringBuilderStream::create();
            const auto streamRedirect = el::stream::redirectStandardStreams(output, error);
            auto scope = ApplicationTestScope<Application>{1, argv};
            auto &application = scope.app();
            auto mainCalled = false;
            const auto module = OptionModule::create("run"_el);
            module->setHelpDescription("Run the configured operation."_el);
            application.options()->addModule(module);
            application.setMainFn([&mainCalled]() -> ExitCode {
                mainCalled = true;
                return ExitCode::success();
            });

            REQUIRE_EQUAL(application.run(), 0);
            const auto text = el::text::StringConverter{output->toU8String()}.toStdString();
            REQUIRE_NOT_EQUAL(text.find("tool <module> [options]"), std::string::npos);
            REQUIRE_NOT_EQUAL(text.find("run"), std::string::npos);
            REQUIRE_EQUAL(text.find("Options:"), std::string::npos);
            REQUIRE_FALSE(application.optionValues());
            REQUIRE_FALSE(mainCalled);
        }
    }

    void testApplicationMasksNarrowSensitiveArgumentsOnSuccessAndError() {
        const auto streamRedirect = redirectStandardStreams();
        {
            char arg0[] = "tool";
            char arg1[] = "--secret=long-secret";
            char *argv[] = {arg0, arg1};
            auto scope = ApplicationTestScope<Application>{2, argv};
            auto &application = scope.app();
            application.options()->addOption("--secret"_el).setType(OptionType::SensitiveText);
            application.setMainFn([]() -> ExitCode { return ExitCode::success(); });

            REQUIRE_EQUAL(application.run(), 0);
            REQUIRE_EQUAL(application.commandLineArguments().get(ItemIndex{1U}), "--secret=*****"_el);
            REQUIRE_EQUAL(std::string_view{arg1}, "--secret=***********");
        }

        char arg0[] = "tool";
        char arg1[] = "--secret";
        char arg2[] = "rejected-secret";
        char *argv[] = {arg0, arg1, arg2};
        auto scope = ApplicationTestScope<Application>{3, argv};
        auto &application = scope.app();
        application.options()
            ->addOption("--secret"_el)
            .setType(OptionType::SensitiveText)
            .setValidateFn([](OptionValuePtr, OptionValuesPtr) -> void {
                throw OptionError{OptionErrorContext{}.setDescription("Rejected secret"_el)};
            });

        REQUIRE_NOT_EQUAL(application.run(), 0);
        REQUIRE_EQUAL(application.commandLineArguments().get(ItemIndex{2U}), "*****"_el);
        REQUIRE_EQUAL(std::string_view{arg2}, "***************");
    }

    void testApplicationMasksWideSensitiveArgumentsOnSuccessAndError() {
        const auto streamRedirect = redirectStandardStreams();
        {
            wchar_t arg0[] = L"tool";
            wchar_t arg1[] = L"--secret";
            wchar_t arg2[] = L"秘密値";
            wchar_t *argv[] = {arg0, arg1, arg2};
            auto scope = ApplicationTestScope<Application>{3, argv};
            auto &application = scope.app();
            application.options()->addOption("--secret"_el).setType(OptionType::SensitiveText);
            application.setMainFn([]() -> ExitCode { return ExitCode::success(); });

            REQUIRE_EQUAL(application.run(), 0);
            REQUIRE_EQUAL(application.commandLineArguments().get(ItemIndex{2U}), "*****"_el);
            REQUIRE_EQUAL(std::wstring_view{arg2}, L"***");
        }

        wchar_t arg0[] = L"tool";
        wchar_t arg1[] = L"--secret=wide-error";
        wchar_t *argv[] = {arg0, arg1};
        auto scope = ApplicationTestScope<Application>{2, argv};
        auto &application = scope.app();
        application.options()
            ->addOption("--secret"_el)
            .setType(OptionType::SensitiveText)
            .setValidateFn([](OptionValuePtr, OptionValuesPtr) -> void {
                throw OptionError{OptionErrorContext{}.setDescription("Rejected secret"_el)};
            });

        REQUIRE_NOT_EQUAL(application.run(), 0);
        REQUIRE_EQUAL(application.commandLineArguments().get(ItemIndex{1U}), "--secret=*****"_el);
        REQUIRE_EQUAL(std::wstring_view{arg1}, L"--secret=**********");
    }

    void testApplicationPreservesWideUnicodeArguments() {
        wchar_t arg0[] = L"resource-compiler";
        wchar_t arg1[] = L"--input";
        wchar_t arg2[] = L"C:\\fixtures\\nested folder\\caf\u00e9_\u65e5\u672c\u8a9e.json";
        wchar_t *argv[] = {arg0, arg1, arg2};

        auto scope = ApplicationTestScope<Application>{3, argv};
        auto &application = scope.app();

        REQUIRE_EQUAL(application.commandLineArguments().count().toSizeT(), 3U);
        REQUIRE_EQUAL(
            application.commandLineArguments().get(ItemIndex{2U}),
            u8"C:\\fixtures\\nested folder\\caf\u00e9_\u65e5\u672c\u8a9e.json"_el);
    }

private:
    [[nodiscard]] static auto redirectStandardStreams() -> el::stream::StandardStreamRedirect {
        return el::stream::redirectStandardStreams(
            el::stream::AnyStringBuilderStream::create(), el::stream::AnyStringBuilderStream::create());
    }

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
        REQUIRE_FALSE(application.optionValues());
        REQUIRE_FALSE(moduleMainCalled);
        REQUIRE_FALSE(applicationMainCalled);
    }

    void runErrorCase(char *argv[], bool &moduleMainCalled, bool &applicationMainCalled) {
        auto scope = ApplicationTestScope<Application>{3, argv};
        auto &application = scope.app();
        auto module = OptionModule::create("run"_el);
        module->addOption("--name"_el).setType(OptionType::Text);
        configureSkippedMainTest(application, module, moduleMainCalled, applicationMainCalled);

        REQUIRE_NOT_EQUAL(application.run(), 0);
        REQUIRE_FALSE(application.optionValues());
        REQUIRE_FALSE(moduleMainCalled);
        REQUIRE_FALSE(applicationMainCalled);
    }
};
