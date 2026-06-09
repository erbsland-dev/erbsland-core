// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../cterm/support/TerminalTestBackend.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/options/OptionHelp.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/stream/StringBuilderStream.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <string>
#include <utility>

using namespace el::text::literals;

TESTED_TARGETS(Application TerminalOptionsRenderer TerminalStream)
class ApplicationTerminalTest final : public el::UnitTest {
    class TestApplication final : public el::core::Application {
    public:
        explicit TestApplication(bool interactive) : Application{} {
            backend->_isInteractive = interactive;
            backend->_supportsColorCodes = false;
        }

    public:
        std::shared_ptr<TerminalTestBackend> backend = std::make_shared<TerminalTestBackend>();

    protected:
        [[nodiscard]] auto createAndInitializeTerminal() -> el::cterm::TerminalPtr override {
            auto result = std::make_shared<el::cterm::Terminal>(backend, bgeo::BlockSize{60, 25});
            result->initializeScreen();
            return result;
        }
    };

public:
    void testInteractiveTerminalRedirectsCapturedStandardStreamPointer() {
        const auto capturedOutput = el::stream::stdOut();
        std::string terminalOutput;

        {
            auto application = TestApplication{true};
            application.enableTerminal();

            capturedOutput->writeLine("through terminal"_el);
            capturedOutput->flush();

            terminalOutput = application.backend->output();
        }

        requireContains(terminalOutput, "through terminal\n");
    }

    void testNonInteractiveTerminalKeepsExistingStandardStreams() {
        const auto replacement = el::stream::StringBuilderStream::create();
        auto redirect = el::stream::redirectStdOut(replacement);

        auto application = TestApplication{false};
        application.enableTerminal();

        el::stream::stdOut()->writeLine("plain output"_el);

        REQUIRE_EQUAL(toStdString(replacement), std::string{"plain output\n"});
        requireMissing(application.backend->output(), "plain output");
    }

    void testInteractiveHelpUsesTerminalRenderer() {
        char arg0[] = "tool";
        char arg1[] = "--help";
        char *argv[] = {arg0, arg1};

        auto help = el::options::OptionHelp{"Terminal help text."_el};
        help.setTitle("Tool"_el);
        auto backend = std::make_shared<TerminalTestBackend>();

        auto argumentApplication = ArgumentApplication{2, argv, backend};
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

private:
    class ArgumentApplication final : public el::core::Application {
    public:
        ArgumentApplication(int argc, char *argv[], std::shared_ptr<TerminalTestBackend> backend) :
            Application{argc, argv}, backend{std::move(backend)} {
            this->backend->_isInteractive = true;
            this->backend->_supportsColorCodes = false;
        }

    public:
        std::shared_ptr<TerminalTestBackend> backend;

    protected:
        [[nodiscard]] auto createAndInitializeTerminal() -> el::cterm::TerminalPtr override {
            auto result = std::make_shared<el::cterm::Terminal>(backend, bgeo::BlockSize{60, 25});
            result->initializeScreen();
            return result;
        }
    };

private:
    [[nodiscard]] static auto toStdString(const el::stream::StringBuilderStreamPtr &stream) -> std::string {
        return el::text::StringConverter{stream->toU8String()}.toStdString();
    }

    void requireContains(const std::string &text, const std::string &needle) {
        REQUIRE(text.find(needle) != std::string::npos);
    }

    void requireMissing(const std::string &text, const std::string &needle) {
        REQUIRE(text.find(needle) == std::string::npos);
    }
};
