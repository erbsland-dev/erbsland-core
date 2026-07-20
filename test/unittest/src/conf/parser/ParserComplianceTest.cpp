// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormatForConf.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdlib>
#include <filesystem>
#include <format>

using namespace el::conf;

TESTED_TARGETS(Parser)
class ParserComplianceTest final : public el::UnitTest {
public:
    static constexpr auto cTestSuiteEnv = "ERBSLAND_CORE_CONF_TEST_SUITE";
    static constexpr auto cTestSuiteDir = "test/erbsland-lang-config-tests";
    static constexpr auto cTestSuiteSubdir = "tests/V1_0";

    std::filesystem::path testSuitePath;
    std::filesystem::path testFilePath;
    SourcePtr source;
    DocumentPtr document;

    auto additionalErrorMessages() -> std::string override {
        try {
            std::string result;
            result += std::format(
                "Failed test file path: {}\n", std::filesystem::relative(testFilePath, testSuitePath).string());
            if (document != nullptr) {
                result += "State of the parsed document:\n";
                auto flatMap = document->toFlatValueMap();
                for (const auto &[namePath, value] : flatMap) {
                    result += el::text::StringConverter{namePath.toText()}.toStdString() + ": " +
                        el::text::StringConverter{value->toTestText()}.toStdString() + "\n";
                }
            } else {
                result += "No document was parsed.\n";
            }
            return result;
        } catch (...) {
            return {"unexpected exception"};
        }
    }

    void validateTestFile(const std::filesystem::path &path, bool expectPass) {
        source = Source::fromFile(el::path::Path{path});
        try {
            Parser parser;
            document = {};
            document = parser.parseOrThrow(source);
            if (!expectPass) {
                consoleWriteLine("Parsing file should have failed.");
                REQUIRE(expectPass);
            }
        } catch (const ConfError &error) {
            if (expectPass) {
                consoleWriteLine(el::text::StringConverter{error.toString()}.toStdString());
                REQUIRE_FALSE(expectPass);
            }
        }
    }

    void testPassOrFail() {
        const bool suiteExplicitlyConfigured = (std::getenv(cTestSuiteEnv) != nullptr);
        if (const auto testSuiteEnv = std::getenv(cTestSuiteEnv); testSuiteEnv != nullptr) {
            testSuitePath = std::filesystem::path(testSuiteEnv);
        } else {
            // If no environment variable is set, the unittest wasn't started using CTest.
            // In this case, we make a guess about the location, assuming this unittest runs in an IDE
            // and the build directory is located inside the project directory.
            auto guessedPath = std::filesystem::path(unitTestExecutablePath()).parent_path();
            int maxDepth = 5;
            while (is_directory(guessedPath) && maxDepth-- > 0) {
                if (is_directory(guessedPath / cTestSuiteDir)) {
                    testSuitePath = guessedPath / cTestSuiteDir;
                    break;
                }
                guessedPath = guessedPath.parent_path();
            }
        }
        if (testSuitePath.empty() || !std::filesystem::is_directory(testSuitePath)) {
            consoleWriteLine(
                std::format(
                    "Parser compliance test suite directory was not found: {}\n"
                    "Set {} to the local checkout of the compliance test suite to enable this test.",
                    testSuitePath.empty() ? std::string{"<empty>"} : testSuitePath.string(),
                    cTestSuiteEnv));
            // Only fail if the suite location was explicitly configured. Otherwise, allow
            // IDE/local runs without the external test suite checkout.
            if (suiteExplicitlyConfigured) {
                REQUIRE(false);
            }
            return;
        }
        testSuitePath /= cTestSuiteSubdir;
        REQUIRE(std::filesystem::is_directory(testSuitePath));
        std::filesystem::recursive_directory_iterator it(testSuitePath);
        std::vector<std::filesystem::directory_entry> entries;
        for (const auto &entry : it) {
            if (entry.is_regular_file() && entry.path().extension() == ".elcl") {
                entries.push_back(entry);
            }
        }
        std::ranges::sort(entries, [](const auto &a, const auto &b) { return a.path() < b.path(); });
        for (const auto &entry : entries) {
            testFilePath = entry.path();
            bool expectPass = testFilePath.filename().string().find("PASS") != std::string::npos;
            WITH_CONTEXT(validateTestFile(entry.path(), expectPass));
        }
    }
};
