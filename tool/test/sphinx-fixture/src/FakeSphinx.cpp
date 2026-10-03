// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

/// Small external executable fixture; native strings model Sphinx's external process boundary.
/// @notest{External executable fixture exercised by BuildDocToolTest.}
class FakeSphinx final {
public:
    /// Validate native invocation and emit deterministic diagnostics.
    /// @param argc Argument count.
    /// @param argv Native arguments.
    /// @return Simulated Sphinx status.
    static auto run(const int argc, char *argv[]) -> int {
        const auto root = std::filesystem::current_path();
        const auto environment = std::getenv("VIRTUAL_ENV");
        if (argc != 5 || std::string_view{argv[1]} != "-j" || std::string_view{argv[2]} != "1" ||
            std::string_view{argv[3]} != "doc" || std::string_view{argv[4]} != "_build") {
            std::cerr << "Unexpected Sphinx arguments\n";
            return 90;
        }
        // Windows can spell the same directory using an extended prefix or a different drive-letter case.
        auto error = std::error_code{};
        if (environment == nullptr || !std::filesystem::equivalent(environment, root / ".venv", error)) {
            std::cerr << "Unexpected VIRTUAL_ENV: " << (environment == nullptr ? "<missing>" : environment)
                      << "; working directory: " << root.generic_string() << '\n';
            return 90;
        }
        std::ofstream{root / "fake-sphinx-ran"} << "yes";
        auto mode = std::string{};
        std::ifstream{root / "fake-sphinx-mode"} >> mode;
        std::cout << "reading sources...\n";
        std::cout << root.generic_string() << "/_doxygen_input/erbsland/impl/Hidden.hpp:9: warning: allowed\n";
        std::cerr << root.generic_string() << "/doc/page.rst:12: WARNING: useful diagnostic\n";
        if (mode == "failure") {
            std::cout << "Sphinx build error:\nTraceback detail without a diagnostic prefix\n";
            return 2;
        }
        return 0;
    }
};

auto main(const int argc, char *argv[]) -> int {
    return FakeSphinx::run(argc, argv);
}
