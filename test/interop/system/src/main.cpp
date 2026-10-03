// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <chrono>
#include <csignal>
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
        if (mode == "utf8-boundaries") {
            auto block = std::array<char, 259>{};
            block.fill('a');
            block[255] = static_cast<char>(0xF0);
            block[256] = static_cast<char>(0x9F);
            block[257] = static_cast<char>(0x92);
            block[258] = static_cast<char>(0xA1);
            for (auto index = 0; index < 128; ++index) {
                std::cout.write(block.data(), static_cast<std::streamsize>(block.size()));
            }
            return 0;
        }
        if (mode == "utf8-output") {
            const auto bytes = std::array<unsigned char, 15>{
                0xEF, 0xBB, 0xBF, 'a', 0xF0, 0x9F, 0x92, 0xA1, 0xFF, 0, 'b', 0xE2, 0x82, 0xAC, 0xE2};
            for (const auto byte : bytes) {
                std::cout.put(static_cast<char>(byte));
                std::cout.flush();
            }
            return 0;
        }
        if (mode == "phased-output") {
            std::cout << "first" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds{200});
            std::cout << "second" << std::flush;
            return 0;
        }
        if (mode == "crash") {
#ifdef _WIN32
            std::_Exit(static_cast<int>(0xC0000005U));
#else
            std::raise(SIGKILL);
#endif
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
        if (mode == "environment-marker") {
            if (argc < 5) {
                return 64;
            }
            auto marker = std::ofstream{std::filesystem::path{argv[3]}, std::ios::binary};
            const auto *value = std::getenv(argv[4]);
            const auto *path = std::getenv("PATH");
            if (value != nullptr) {
                marker << value;
            }
            marker << '\n';
            if (path != nullptr) {
                marker << path;
            }
            return marker ? 0 : 1;
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
