// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/Path.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringJoins.hpp>

#include <string_view>

using namespace el::text::literals;

/// Emulate Git and zizmor deterministically using files in the test project.
class MaintenanceFixture final {
public:
    /// Run the requested fixture operation.
    /// @param argc Native argument count.
    /// @param argv Native argument vector.
    /// @return The configured tool exit code.
    static auto run(const int argc, char *argv[]) -> int {
        if (argc < 2) {
            return 90;
        }
        const auto command = el::String{std::string_view{argv[1]}};
        const auto calls = el::Path{".fixture-calls"_el};
        auto previous = el::String{};
        if (calls.info().exists()) {
            previous = calls.content().readTextOrThrow();
        }
        calls.content().writeTextOrThrow(
            el::join(previous, command, "\n"_el),
            el::PathWriteTextOptions{}.setCreationMode(el::PathCreateMode::CreateOrOverwrite));
        if (command == "ls-remote"_el) {
            if (argc != 4 || el::String{std::string_view{argv[2]}} != "--tags"_el) {
                return 91;
            }
            if (el::Path{".fixture-truncated"_el}.info().exists()) {
                const auto chunk = el::String::fromCharacter(U'x', el::CpCount{4096});
                for (auto index = 0; index < 9 * 256; ++index) {
                    if (el::stream::stdOut()->write(chunk).isFailure() || el::stream::stdOut()->flush().isFailure()) {
                        return 93;
                    }
                }
                return 0;
            }
            const auto tags = el::Path{".fixture-tags"_el};
            if (!tags.info().exists()) {
                el::stream::stdErr()->write("remote unavailable\n"_el);
                return 2;
            }
            el::stream::stdOut()->write(tags.content().readTextOrThrow());
            return 0;
        }
        if (command == "ls-files"_el) {
            if (el::Path{".fixture-tracked"_el}.info().exists()) {
                return 0;
            }
            return 1;
        }
        if (command == "check-ignore"_el) {
            if (el::Path{".fixture-ignored"_el}.info().exists()) {
                return 0;
            }
            return 1;
        }
        if (command == "--offline"_el) {
            const auto status = el::Path{".fixture-zizmor"_el};
            if (status.info().exists()) {
                return status.content().readTextOrThrow().trimmed().toIntegerOrThrow<int>();
            }
            return 0;
        }
        return 92;
    }
};

/// Dispatch the compiled external-command fixture.
auto main(const int argc, char *argv[]) -> int {
    const auto result = MaintenanceFixture::run(argc, argv);
    el::stream::stdOut()->flush();
    el::stream::stdErr()->flush();
    return result;
}
