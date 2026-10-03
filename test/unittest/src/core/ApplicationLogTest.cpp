// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include "../cterm/support/TerminalTestBackend.hpp"
#include "../i18n/DisplayTextTestLayer.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/cterm/BlockAttributes.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/DisplayText.hpp>
#include <erbsland/LogConfiguration.hpp>
#include <erbsland/LogManager.hpp>
#include <erbsland/LogStream.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <string>

using namespace el::text::literals;

namespace application_log_test {

class LastErrorDumpApplication final : public el::core::Application {
public:
    LastErrorDumpApplication() {
        backend->_isInteractive = true;
        backend->_supportsColorCodes = false;
        backend->_supportedBlockAttributes = el::cterm::BlockAttributes::fromMask(0);
        backend->_supportedBlockAttributeCodes = el::cterm::BlockAttributes::fromMask(0);
    }

public:
    el::unit::ExitCode result = el::unit::ExitCode::success();
    std::shared_ptr<TerminalTestBackend> backend = std::make_shared<TerminalTestBackend>();

protected:
    [[nodiscard]] auto main() -> el::unit::ExitCode override {
        logStream()->error("A retained test error."_el);
        return result;
    }

    [[nodiscard]] auto createAndInitializeTerminal(
        [[maybe_unused]] el::block::Size size, [[maybe_unused]] el::cterm::TerminalFlags flags)
        -> el::cterm::TerminalPtr override {
        auto terminal = std::make_shared<el::cterm::Terminal>(backend, el::block::Size{80, 25});
        terminal->initializeScreen();
        return terminal;
    }
};

}

using application_log_test::LastErrorDumpApplication;

TESTED_TARGETS(Application)
class ApplicationLogTest final : public el::UnitTest {
public:
    void testLogManagerAndRootStreamAreCreatedLazily() {
        auto scope = ApplicationTestScope<>{};
        auto &application = scope.app();

        auto &manager = application.log();
        REQUIRE_EQUAL(&manager, &application.log());
        REQUIRE_EQUAL(application.logStream(), manager.rootStream());
        REQUIRE_EQUAL(manager.configuration().writerCount(), std::size_t{1U});
    }

    void testLastErrorDumpWriterSurvivesConfigurationReplacement() {
        auto scope = ApplicationTestScope<>{};
        auto &application = scope.app();
        application.enableLastErrorDump();
        application.enableLastErrorDump();
        application.log().setConfiguration({});

        const auto configuration = application.log().configuration();
        REQUIRE_EQUAL(configuration.writerCount(), std::size_t{1U});
    }

    void testLastErrorDumpIsDisplayedAfterFailureWithLocalizedTitle() {
        auto backend = std::shared_ptr<TerminalTestBackend>{};
        {
            auto scope = ApplicationTestScope<LastErrorDumpApplication>{};
            auto &application = scope.app();
            backend = application.backend;
            application.result = el::unit::ExitCode{17};
            const auto displayText = std::make_shared<DisplayTextTestData>(std::initializer_list<DisplayTextTestEntry>{
                {"log"_el, "last_error_dump_title"_el, "Retained Test Errors"_el},
            });
            application.displayText()->addData(displayText);

            application.enableLastErrorDump();

            REQUIRE_EQUAL(application.run(), 17);
        }

        requireContains(backend->output(), "Retained Test Errors");
    }

    void testLastErrorDumpIsHiddenAfterSuccessByDefault() {
        auto backend = std::shared_ptr<TerminalTestBackend>{};
        {
            auto scope = ApplicationTestScope<LastErrorDumpApplication>{};
            auto &application = scope.app();
            backend = application.backend;
            application.enableLastErrorDump();

            REQUIRE_EQUAL(application.run(), 0);
        }

        requireMissing(backend->output(), "Recent Error Log Entries");
    }

    void testLastErrorDumpCanBeDisplayedAfterSuccess() {
        auto backend = std::shared_ptr<TerminalTestBackend>{};
        {
            auto scope = ApplicationTestScope<LastErrorDumpApplication>{};
            auto &application = scope.app();
            backend = application.backend;
            application.enableLastErrorDump();
            application.enableLastErrorDump(el::core::LastErrorDumpMode::Always);

            REQUIRE_EQUAL(application.run(), 0);
        }

        requireContains(backend->output(), "Recent Error Log Entries");
    }

private:
    void requireContains(const std::string &text, const std::string &needle) {
        REQUIRE_NOT_EQUAL(text.find(needle), std::string::npos);
    }

    void requireMissing(const std::string &text, const std::string &needle) {
        REQUIRE_EQUAL(text.find(needle), std::string::npos);
    }
};
