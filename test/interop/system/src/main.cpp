// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

class SubprocessHelper final {
public:
    [[nodiscard]] static auto run(const int argc, char *argv[]) -> std::optional<int> {
        if (argc < 3 || std::string{argv[1]} != "--subprocess-helper") {
            return std::nullopt;
        }
        const auto mode = std::string{argv[2]};
        if (mode == "output") {
            std::cout << (argc >= 4 ? argv[3] : "stdout") << std::flush;
            std::cerr << (argc >= 5 ? argv[4] : "stderr") << std::flush;
            return argc >= 6 ? std::atoi(argv[5]) : 0;
        }
        if (mode == "large-output") {
            const auto count = argc >= 4 ? std::strtoull(argv[3], nullptr, 10) : 0U;
            for (auto index = std::size_t{}; index < count; ++index) {
                std::cout.put(static_cast<char>('a' + index % 26U));
            }
            return 0;
        }
        if (mode == "sleep") {
            const auto milliseconds = argc >= 4 ? std::strtoll(argv[3], nullptr, 10) : 1000;
            std::this_thread::sleep_for(std::chrono::milliseconds{milliseconds});
            return 0;
        }
        if (mode == "environment") {
            const auto *value = argc >= 4 ? std::getenv(argv[3]) : nullptr;
            std::cout << (value == nullptr ? "<missing>" : value) << std::flush;
            return 0;
        }
        if (mode == "working-directory") {
            std::cout << std::filesystem::current_path().string() << std::flush;
            return 0;
        }
        if (mode == "marker-after") {
            const auto milliseconds = argc >= 4 ? std::strtoll(argv[3], nullptr, 10) : 1000;
            std::this_thread::sleep_for(std::chrono::milliseconds{milliseconds});
            if (argc < 5) {
                return 64;
            }
            auto marker = std::ofstream{std::filesystem::path{argv[4]}};
            marker << "created";
            return marker ? 0 : 1;
        }
        return 64;
    }
};

auto main(const int argc, char *argv[]) -> int {
    if (const auto helperResult = SubprocessHelper::run(argc, argv)) {
        return *helperResult;
    }
    return erbsland::unittest::Controller::instance()->main(argc, argv);
}
