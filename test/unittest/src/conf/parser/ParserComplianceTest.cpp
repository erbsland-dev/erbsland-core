// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/Parser.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathWalker.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <cstdlib>
#include <exception>
#include <format>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace el::conf;
using el::path::Path;

TESTED_TARGETS(Parser)
class ParserComplianceTest final : public el::UnitTest {
public:
    static constexpr auto cTestSuiteEnv = "ERBSLAND_CORE_CONF_TEST_SUITE"_el;
    static constexpr auto cTestSuiteDir = "test/erbsland-lang-config-tests"_el;
    static constexpr auto cTestSuiteSubdir = "tests/V1_0"_el;
    static constexpr auto cWorkerCount = std::size_t{32};

    Path testSuitePath;
    Path testFilePath;

    auto additionalErrorMessages() -> std::string override {
        try {
            return std::format("Failed test file path: {}\n", testFilePath.toRelative(testSuitePath).toString());
        } catch (...) {
            return {"unexpected exception"};
        }
    }

    static void validateTestFile(const Path &path, bool expectPass) {
        const auto source = Source::fromFile(path);
        try {
            Parser parser;
            parser.parseOrThrow(source);
            if (!expectPass) {
                throw std::runtime_error{"Parsing file should have failed."};
            }
        } catch (const ConfError &) {
            if (expectPass) {
                throw;
            }
        }
    }

    void runTestFiles(const el::List<Path> &paths) {
        const auto &rawPaths = paths.toRawValue();
        auto nextIndex = std::atomic_size_t{};
        auto failures = std::vector<std::exception_ptr>(rawPaths.size());
        auto workers = std::vector<std::jthread>{};
        workers.reserve(cWorkerCount);
        for (auto workerIndex = std::size_t{}; workerIndex < cWorkerCount; ++workerIndex) {
            workers.emplace_back([&rawPaths, &nextIndex, &failures]() -> void {
                while (true) {
                    const auto index = nextIndex.fetch_add(1, std::memory_order_relaxed);
                    if (index >= rawPaths.size()) {
                        return;
                    }
                    try {
                        const auto &path = rawPaths[index];
                        validateTestFile(path, path.name().contains("PASS"_el));
                    } catch (...) {
                        failures[index] = std::current_exception();
                    }
                }
            });
        }
        for (auto &worker : workers) {
            worker.join();
        }
        for (auto index = std::size_t{}; index < failures.size(); ++index) {
            if (failures[index] != nullptr) {
                testFilePath = rawPaths[index];
                std::rethrow_exception(failures[index]);
            }
        }
    }

    void testPassOrFail() {
        const auto testSuiteEnvPtr = std::getenv(el::StringConverter{cTestSuiteEnv}.toStdString().c_str());
        const bool suiteExplicitlyConfigured = (testSuiteEnvPtr != nullptr);
        if (testSuiteEnvPtr != nullptr) {
            testSuitePath = Path(el::String{std::string_view{testSuiteEnvPtr}});
        } else {
            // If no environment variable is set, the unittest wasn't started using CTest.
            // In this case, we make a guess about the location, assuming this unittest runs in an IDE
            // and the build directory is located inside the project directory.
            auto guessedPath = Path(unitTestExecutablePath()).parent();
            int maxDepth = 5;
            while (guessedPath.info().isDirectory() && maxDepth-- > 0) {
                auto newPath = guessedPath / cTestSuiteDir;
                if (newPath.info().isDirectory()) {
                    testSuitePath = newPath;
                    break;
                }
                guessedPath = guessedPath.parent();
            }
        }
        if (testSuitePath.isEmpty() || !testSuitePath.info().isDirectory()) {
            consoleWriteLine(
                std::format(
                    "Parser compliance test suite directory was not found: {}\n"
                    "Set {} to the local checkout of the compliance test suite to enable this test.",
                    testSuitePath.isEmpty() ? el::String{"<empty>"_el} : testSuitePath.toString(),
                    el::String{cTestSuiteEnv}));
            // Only fail if the suite location was explicitly configured. Otherwise, allow
            // IDE/local runs without the external test suite checkout.
            if (suiteExplicitlyConfigured) {
                REQUIRE(false);
            }
            return;
        }
        testSuitePath /= cTestSuiteSubdir;
        REQUIRE(testSuitePath.info().isDirectory());
        el::List<Path> paths;
        testSuitePath.walker().walkOrThrow(
            [&](const Path &path) -> el::PathWalkStatus {
                if (path.suffix() == ".elcl"_el) {
                    paths.append(path);
                }
                return el::PathWalkStatus::Continue;
            },
            el::PathWalkOptions{}.setTypes(el::PathType::RegularFile));
        WITH_CONTEXT(runTestFiles(paths));
    }
};
