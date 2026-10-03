// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Application.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/cterm/TerminalFlag.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/StringFormat.hpp>

#include <chrono>
#include <thread>

using namespace el::text::literals;

/// Portable child process exercising virtual terminal output and capture limits.
class CaptureFixture final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->addOption({"mode"_el}).setType(el::OptionType::Text).setFlag(el::OptionFlag::Required);
    }
    auto main() -> el::ExitCode override {
        if (commandLineArguments().count() != el::ItemCount{2}) {
            return el::ExitCode{2};
        }
        const auto mode = commandLineArguments().last();
        if (mode == "timeout"_el) {
            std::this_thread::sleep_for(std::chrono::seconds{20});
            return el::ExitCode::success();
        }
        if (mode == "interactive"_el) {
            const auto environment = el::EnvironmentVariables{};
            if (!environment.get("ERBSLAND_VIRTUAL_TTY"_el) || environment.get("ERBSLAND_CAPTURE_SECRET"_el)) {
                return el::ExitCode{4};
            }
            const auto dimensions = environment.getOrThrow("ERBSLAND_VIRTUAL_TTY_SIZE"_el).splitAtFirst("x"_el);
            if (!dimensions) {
                return el::ExitCode{5};
            }
            enableTerminal(
                el::block::Size{dimensions->first.toIntegerOrThrow<int>(), dimensions->second.toIntegerOrThrow<int>()},
                el::cterm::TerminalFlag::VirtualTty);
            if (!terminal()->isInteractive() || !el::stdIn()->readLine().isFinished()) {
                return el::ExitCode{3};
            }
            const auto size = terminal()->size();
            enableTerminal(el::block::Size{1, 1}); // Repeated calls retain the original terminal and dimensions.
            if (terminal()->size() != size) {
                return el::ExitCode{8};
            }
            terminal()->printLine(el::cterm::fg::Red, "terminal hé界"_el);
            el::stdOut()->writeLine(
                el::StringFormat{"size={}x{}"_el}.build(size.width().toRawValue(), size.height().toRawValue()));
            el::stdErr()->writeLine("merged error"_el);
            el::stdOut()->flush();
            el::stdErr()->flush();
            terminal()->setAlternateScreen(true);
            terminal()->moveTo({0, 0});
            terminal()->printLine("alternate"_el);
            terminal()->setAlternateScreen(false);
            terminal()->input().setMode(el::cterm::Input::Mode::Key);
            if (terminal()->input().readKey().valid() || terminal()->input().waitForKey().valid() ||
                !terminal()->input().readLine().isEmpty()) {
                return el::ExitCode{6};
            }
            return el::ExitCode::success();
        }
        auto count = 64;
        if (mode == "limit"_el) {
            count = 512;
        }
        const auto chunk = el::String::fromCharacter(U'x', el::CpCount{4096});
        for (auto index = 0; index < count; ++index) {
            el::stdOut()->write(chunk);
        }
        el::stdOut()->flush();
        return el::ExitCode{7};
    }
};

auto main(const int argc, char *argv[]) -> int {
    auto application = CaptureFixture{argc, argv};
    return application.run();
}
