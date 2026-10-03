// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include "../cterm/support/TerminalTestBackend.hpp"

#include <erbsland/AnyStringBuilderStream.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/core/impl/application_data/ApplicationTerminalData.hpp>
#include <erbsland/core/impl/ApplicationDataImpl.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionHelp.hpp>
#include <erbsland/OptionModule.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/stream/impl/NativeOutputStream.hpp>
#include <erbsland/stream/impl/StandardTextOutputStream.hpp>
#include <erbsland/StreamError.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <span>
#include <string>
#include <utility>

using namespace el::text::literals;

TESTED_TARGETS(Application ApplicationDataImpl TerminalDocumentRenderer TerminalStream)
class ApplicationTerminalTest final : public el::UnitTest {
    /// Record writes and flushes performed by the asynchronous standard stream.
    class RecordingNativeOutputStream final : public el::stream::impl::NativeOutputStream {
    public: // implement NativeOutputStream
        void writeBytes(const std::span<const char> bytes) override { writtenBytes += bytes.size(); }
        void flush() override {
            ++flushCount;
            if (failOnFlush) {
                throw el::stream::StreamError{
                    el::stream::StreamErrorContext{"Test flush failure."_el, "Configured native flush failure."_el}};
            }
        }
        void abort() noexcept override {}

    public:
        std::atomic<std::size_t> writtenBytes{}; ///< Bytes delivered to the native stream.
        std::atomic<int> flushCount{};           ///< Native flush calls.
        bool failOnFlush{false};                 ///< Whether native flush should fail.
    };

    class TestApplication final : public el::core::Application {
    public:
        TestApplication() = default;

    public:
        void setInteractive(bool interactive) {
            backend->_isInteractive = interactive;
            backend->_supportsColorCodes = false;
        }

    public:
        std::shared_ptr<TerminalTestBackend> backend = std::make_shared<TerminalTestBackend>();
        el::block::Size requestedSize{};
        el::cterm::TerminalFlags requestedFlags{};
        int creationCount{0};

    protected:
        [[nodiscard]] auto createAndInitializeTerminal(el::block::Size size, el::cterm::TerminalFlags flags)
            -> el::cterm::TerminalPtr override {
            requestedSize = size;
            requestedFlags = flags;
            ++creationCount;
            auto result = std::make_shared<el::cterm::Terminal>(backend, block::Size{60, 25});
            result->initializeScreen();
            return result;
        }
    };

public:
    void testTerminalCreationReceivesDimensionsAndFlagsOnce() {
        auto scope = ApplicationTestScope<TestApplication>{};
        auto &application = scope.app();
        const auto flags = el::cterm::TerminalFlag::VirtualTty | el::cterm::TerminalFlag::NoSignalHandling;
        application.enableTerminal(el::block::Size{90, 40}, flags);
        REQUIRE_EQUAL(application.requestedSize, el::block::Size(90, 40));
        REQUIRE(application.requestedFlags.has(el::cterm::TerminalFlag::VirtualTty));
        REQUIRE(application.requestedFlags.has(el::cterm::TerminalFlag::NoSignalHandling));
        application.enableTerminal(el::block::Size{120, 50});
        REQUIRE_EQUAL(application.creationCount, 1);
        REQUIRE_EQUAL(application.requestedSize, el::block::Size(90, 40));
    }

    void testApplicationExitDrainsStandardStreamsWithoutTerminal() {
        WITH_CONTEXT(requireStandardStreamsDrained(false, false));
    }

    void testApplicationExitDrainsStandardStreamsWithNonInteractiveTerminal() {
        WITH_CONTEXT(requireStandardStreamsDrained(true, false));
    }

    void testApplicationExitFlushesErrorStreamAfterOutputFlushFailure() {
        WITH_CONTEXT(requireStandardStreamsDrained(false, true));
    }

    void testInteractiveTerminalRedirectsCapturedStandardStreamPointer() {
        const auto capturedOutput = el::stream::stdOut();
        std::string terminalOutput;

        {
            auto scope = ApplicationTestScope<TestApplication>{};
            auto &application = scope.app();
            application.setInteractive(true);
            application.enableTerminal();

            capturedOutput->writeLine("through terminal"_el);
            capturedOutput->flush();

            terminalOutput = application.backend->output();
        }

        requireContains(terminalOutput, "through terminal\n");
    }

    void testNonInteractiveTerminalKeepsExistingStandardStreams() {
        const auto replacement = el::stream::AnyStringBuilderStream::create();
        auto redirect = el::stream::redirectStdOut(replacement);

        auto scope = ApplicationTestScope<TestApplication>{};
        auto &application = scope.app();
        application.setInteractive(false);
        application.enableTerminal();

        el::stream::stdOut()->writeLine("plain output"_el);

        REQUIRE_EQUAL(toStdString(replacement), std::string{"plain output\n"});
        requireMissing(application.backend->output(), "plain output");
    }

    void testNonInteractiveApplicationLifecycleEmitsNoAnsi() {
        auto backend = std::shared_ptr<TerminalTestBackend>{};
        {
            auto scope = ApplicationTestScope<TestApplication>{};
            auto &application = scope.app();
            application.setInteractive(false);
            backend = application.backend;

            application.enableTerminal();
            const auto output = backend->output();
            REQUIRE_EQUAL(output, std::string{});
        }

        const auto output = backend->output();
        REQUIRE_EQUAL(output, std::string{});
    }

    void testPlainSystemOutputUsesRootErrorStyleOnly() {
        const auto output = el::stream::AnyStringBuilderStream::create();
        const auto error = el::stream::AnyStringBuilderStream::create();
        auto outputRedirect = el::stream::redirectStdOut(output);
        auto errorRedirect = el::stream::redirectStdErr(error);
        auto data = el::core::impl::ApplicationDataImpl{};

        auto regularDocument = el::text::TextDocument{};
        regularDocument.addError("embedded parser error"_el);
        data.terminal().get()->renderSystemOutput(regularDocument);

        REQUIRE_EQUAL(toStdString(output), std::string{"embedded parser error\n"});
        REQUIRE_EQUAL(toStdString(error), std::string{});

        auto errorDocument = el::text::TextDocument{};
        errorDocument.root()->setStyle("error"_el);
        errorDocument.addParagraph()->addText("real error"_el);
        data.terminal().get()->renderSystemOutput(errorDocument);

        REQUIRE_EQUAL(toStdString(error), std::string{"real error\n"});
    }

    void testInteractiveSystemOutputRespectsOutputGuard() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_isInteractive = true;
        const auto terminal = std::make_shared<el::cterm::Terminal>(backend, block::Size{60, 25});
        auto data = el::core::impl::ApplicationTerminalData{};
        data.enable(terminal);
        auto document = el::text::TextDocument{};
        document.addParagraph()->addText("system diagnostic"_el);

        auto started = std::promise<void>{};
        auto ready = started.get_future();
        auto rendering = std::future<void>{};
        auto status = std::future_status{};
        {
            auto guard = terminal->synchronizeOutput();
            rendering = std::async(std::launch::async, [&]() -> void {
                started.set_value();
                data.renderSystemOutput(document);
            });
            ready.wait();
            status = rendering.wait_for(std::chrono::milliseconds{20});
        }
        // Release the guard and join before assertions, including on the failure path.
        rendering.get();
        const auto output = el::String{backend->output()};
        const auto flushCount = backend->_emitFlushCallCount;
        data.cleanup();

        REQUIRE(status == std::future_status::timeout);
        REQUIRE(output.contains("system diagnostic"_el));
        REQUIRE(flushCount > 0);
    }

    void testInteractiveHelpUsesTerminalRenderer() {
        char arg0[] = "tool";
        char arg1[] = "--help";
        char *argv[] = {arg0, arg1};

        auto help = el::options::OptionHelp{"Terminal help text."_el};
        help.setTitle("Tool"_el);
        help.setDescription("Terminal help text."_el);
        auto backend = std::make_shared<TerminalTestBackend>();

        auto scope = ApplicationTestScope<ArgumentApplication>{2, argv};
        auto &argumentApplication = scope.app();
        argumentApplication.setBackend(backend);
        argumentApplication.info().setApplicationName("Tool"_el);
        argumentApplication.options()->setHelp(help);
        argumentApplication.enableTerminal();

        REQUIRE_EQUAL(argumentApplication.run(), 0);
        const auto text = argumentApplication.backend->output();

        requireContains(text, "Usage:");
        requireContains(text, "Options");

        auto foundThemedColor = false;
        for (const auto &color : argumentApplication.backend->_emittedColors) {
            if (color == el::cterm::Color{el::cterm::fg::BrightWhite, el::cterm::bg::Default} ||
                color == el::cterm::Color{el::cterm::fg::BrightCyan, el::cterm::bg::Default}) {
                foundThemedColor = true;
            }
        }
        REQUIRE(foundThemedColor);
    }

    void testInteractiveDetailedHelpUsesTerminalColors() {
        char arg0[] = "tool";
        char arg1[] = "--help=--help";
        char *argv[] = {arg0, arg1};
        auto scope = ApplicationTestScope<ArgumentApplication>{2, argv};
        auto &application = scope.app();
        application.setBackend(std::make_shared<TerminalTestBackend>());
        application.enableTerminal();
        REQUIRE_EQUAL(application.run(), 0);
        requireContains(application.backend->output(), "tool --help[=<keyword>]");
        for (
            const auto foreground :
            {el::cterm::fg::BrightGreen, el::cterm::fg::BrightYellow, el::cterm::fg::BrightCyan}) {
            auto found = false;
            for (const auto &color : application.backend->_emittedColors) {
                if (color.fg() == foreground) {
                    found = true;
                    break;
                }
            }
            REQUIRE(found);
        }
    }

    void testNonInteractiveDetailedHelpUsesPlainOutput() {
        char arg0[] = "tool";
        char arg1[] = "--help=--help";
        char *argv[] = {arg0, arg1};
        const auto output = el::stream::AnyStringBuilderStream::create();
        auto redirect = el::stream::redirectStdOut(output);
        auto scope = ApplicationTestScope<ArgumentApplication>{2, argv};
        auto &application = scope.app();
        application.setBackend(std::make_shared<TerminalTestBackend>());
        application.backend->_isInteractive = false;
        application.enableTerminal();
        REQUIRE_EQUAL(application.run(), 0);
        const auto plain = toStdString(output);
        requireContains(plain, "tool --help[=<keyword>]");
        requireMissing(plain, std::string{1, char{27}});
        REQUIRE(application.backend->_emittedColors.empty());
        REQUIRE(application.backend->output().empty());
    }

    void testHelpSearchResultsRenderInteractivelyAndAsPlainText() {
        for (const auto interactive : {true, false}) {
            char arg0[] = "tool";
            char arg1[] = "--help=generate";
            char *argv[] = {arg0, arg1};
            const auto output = el::stream::AnyStringBuilderStream::create();
            auto redirect = el::stream::redirectStdOut(output);
            auto scope = ApplicationTestScope<ArgumentApplication>{2, argv};
            auto &application = scope.app();
            application.setBackend(std::make_shared<TerminalTestBackend>());
            application.backend->_isInteractive = interactive;
            auto module = el::options::OptionModule::create("generate-data"_el);
            module->setHelpTitle("Generate data"_el);
            application.options()->addModule(module);
            application.enableTerminal();
            REQUIRE_EQUAL(application.run(), 0);
            auto text = toStdString(output);
            if (interactive) {
                text = application.backend->output();
                REQUIRE(!application.backend->_emittedColors.empty());
            } else {
                REQUIRE(application.backend->_emittedColors.empty());
                requireMissing(text, std::string{1, char{27}});
            }
            if (interactive) {
                requireContains(text, "Matching");
                requireContains(text, "help");
                requireContains(text, "generate");
            } else {
                requireContains(text, "Matching help for \"generate\"");
            }
            requireContains(text, "generate-data");
            requireMissing(text, "--help");
            requireContains(text, "Generate data");
        }
    }

private:
    void requireStandardStreamsDrained(const bool enableTerminal, const bool failOnOutputFlush) {
        const auto nativeOutput = std::make_shared<RecordingNativeOutputStream>();
        const auto nativeError = std::make_shared<RecordingNativeOutputStream>();
        nativeOutput->failOnFlush = failOnOutputFlush;
        const auto output = std::make_shared<el::stream::impl::StandardTextOutputStream>(nativeOutput);
        const auto error = std::make_shared<el::stream::impl::StandardTextOutputStream>(nativeError);
        auto redirect = el::stream::redirectStandardStreams(output, error);
        {
            auto scope = ApplicationTestScope<TestApplication>{};
            if (enableTerminal) {
                scope.app().setInteractive(false);
                scope.app().enableTerminal();
            }
            el::stream::stdOut()->writeLine("last output"_el);
            el::stream::stdErr()->writeLine("last error"_el);
        }
        REQUIRE_EQUAL(nativeOutput->writtenBytes.load(), std::size_t{12});
        REQUIRE_EQUAL(nativeError->writtenBytes.load(), std::size_t{11});
        REQUIRE_EQUAL(nativeOutput->flushCount.load(), 1);
        REQUIRE_EQUAL(nativeError->flushCount.load(), 1);
    }

    class ArgumentApplication final : public el::core::Application {
    public:
        ArgumentApplication(int argc, char *argv[]) : Application{argc, argv} {}

    public:
        void setBackend(std::shared_ptr<TerminalTestBackend> newBackend) {
            backend = std::move(newBackend);
            backend->_isInteractive = true;
            backend->_supportsColorCodes = false;
        }

    public:
        std::shared_ptr<TerminalTestBackend> backend = std::make_shared<TerminalTestBackend>();

    protected:
        [[nodiscard]] auto createAndInitializeTerminal(
            [[maybe_unused]] el::block::Size size, [[maybe_unused]] el::cterm::TerminalFlags flags)
            -> el::cterm::TerminalPtr override {
            auto result = std::make_shared<el::cterm::Terminal>(backend, block::Size{60, 25});
            result->initializeScreen();
            return result;
        }
    };

private:
    [[nodiscard]] static auto toStdString(const el::stream::AnyStringBuilderStreamPtr &stream) -> std::string {
        return el::text::StringConverter{stream->toU8String()}.toStdString();
    }

    void requireContains(const std::string &text, const std::string &needle) {
        const auto position = text.find(needle);
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void { REQUIRE_NOT_EQUAL(position, std::string::npos); },
            [&]() -> std::string { return "needle: " + needle + "\ntext:\n" + text; });
    }

    void requireMissing(const std::string &text, const std::string &needle) {
        const auto position = text.find(needle);
        REQUIRE_EQUAL(position, std::string::npos);
    }
};
