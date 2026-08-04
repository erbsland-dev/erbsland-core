// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/path/Path.hpp>
#include <erbsland/system/Subprocess.hpp>
#include <erbsland/system/SubprocessOptions.hpp>
#include <erbsland/system/SubprocessOutputMode.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <thread>

using namespace el::text::literals;

TESTED_TARGETS(Subprocess SubprocessOptions SubprocessExitStatus)
class SubprocessInteropTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto executablePath() -> el::path::Path {
        return el::path::Path{el::unittest::fh::unitTestExecutablePath()};
    }

    [[nodiscard]] static auto captureOptions() -> el::system::SubprocessOptions {
        return el::system::SubprocessOptions{}
            .setInheritStandardInput(false)
            .setStandardOutputMode(el::system::SubprocessOutputMode::Capture)
            .setStandardErrorMode(el::system::SubprocessOutputMode::Capture);
    }

    [[nodiscard]] static auto markerPath(const std::string &label) -> std::filesystem::path {
        return std::filesystem::temp_directory_path() /
            ("erbsland-subprocess-" + label + "-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    }

    [[nodiscard]] static auto discardOptions() -> el::system::SubprocessOptions {
        return el::system::SubprocessOptions{}
            .setInheritStandardInput(false)
            .setStandardOutputMode(el::system::SubprocessOutputMode::Discard)
            .setStandardErrorMode(el::system::SubprocessOutputMode::Discard);
    }

    [[nodiscard]] static auto toString(const std::filesystem::path &path) -> el::text::String {
        return el::text::StringConverter{path.string()}.toString();
    }

public:
    void testExitAndSeparateCapture() {
        auto process = el::system::Subprocess::start(
            executablePath(),
            el::text::StringList{"--subprocess-helper"_el, "output"_el, "hello out"_el, "hello err"_el, "7"_el},
            captureOptions());
        const auto status = process.wait();
        REQUIRE(status.hasExited());
        REQUIRE_FALSE(status.isSuccess());
        REQUIRE_EQUAL(status.exitCode(), std::optional<std::int32_t>{7});
        REQUIRE_EQUAL(process.standardOutput(), "hello out"_el);
        REQUIRE_EQUAL(process.standardError(), "hello err"_el);
        REQUIRE_FALSE(process.wasStandardOutputTruncated());
        REQUIRE_FALSE(process.wasStandardErrorTruncated());
    }

    void testMergedCapture() {
        auto options = captureOptions();
        options.setMergeStandardError(true);
        auto process = el::system::Subprocess::start(
            executablePath(),
            el::text::StringList{"--subprocess-helper"_el, "output"_el, "first"_el, "second"_el},
            options);
        REQUIRE(process.wait().isSuccess());
        REQUIRE_EQUAL(process.standardOutput(), "firstsecond"_el);
        REQUIRE(process.standardError().isEmpty());
    }

    void testCaptureLimitContinuesDraining() {
        auto options = captureOptions();
        options.setCaptureLimit(el::unit::ByteLength{4096U});
        auto process = el::system::Subprocess::start(
            executablePath(), el::text::StringList{"--subprocess-helper"_el, "large-output"_el, "1048576"_el}, options);
        REQUIRE(process.wait().isSuccess());
        REQUIRE_EQUAL(process.standardOutput().length(), el::unit::ByteLength{4096U});
        REQUIRE(process.wasStandardOutputTruncated());
    }

    void testTimedWaitAndTermination() {
        auto process = el::system::Subprocess::start(
            executablePath(), el::text::StringList{"--subprocess-helper"_el, "sleep"_el, "5000"_el}, captureOptions());
        REQUIRE_FALSE(process.wait(el::time::TimeDelta::milliseconds(20)).has_value());
        REQUIRE(process.isRunning());
        process.terminate();
        REQUIRE(process.wait(el::time::TimeDelta::seconds(2)).has_value());
        REQUIRE_FALSE(process.isRunning());
    }

    void testEnvironmentAndWorkingDirectory() {
        auto options = captureOptions();
        options.setEnvironmentVariable("ERBSLAND_SUBPROCESS_TEST"_el, "value with spaces"_el);
        auto process = el::system::Subprocess::start(
            executablePath(),
            el::text::StringList{"--subprocess-helper"_el, "environment"_el, "ERBSLAND_SUBPROCESS_TEST"_el},
            options);
        REQUIRE(process.wait().isSuccess());
        REQUIRE_EQUAL(process.standardOutput(), "value with spaces"_el);

        const auto temporary = std::filesystem::temp_directory_path();
        options.setWorkingDirectory(el::path::Path{temporary});
        process = el::system::Subprocess::start(
            executablePath(), el::text::StringList{"--subprocess-helper"_el, "working-directory"_el}, options);
        REQUIRE(process.wait().isSuccess());
        REQUIRE_EQUAL(
            std::filesystem::weakly_canonical(el::text::StringConverter{process.standardOutput()}.toStdString()),
            std::filesystem::weakly_canonical(temporary));
    }

    void testOwnedDestructionTerminatesChild() {
        const auto marker = markerPath("owned");
        {
            auto process = el::system::Subprocess::start(
                executablePath(),
                el::text::StringList{"--subprocess-helper"_el, "marker-after"_el, "500"_el, toString(marker)},
                discardOptions());
            REQUIRE(process.isRunning());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{600});
        REQUIRE_FALSE(std::filesystem::exists(marker));
    }

    void testDetachedChildCompletesIndependently() {
        const auto marker = markerPath("detached");
        el::system::Subprocess::startDetached(
            executablePath(),
            el::text::StringList{"--subprocess-helper"_el, "marker-after"_el, "20"_el, toString(marker)},
            discardOptions());
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
        while (!std::filesystem::exists(marker) && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
        REQUIRE(std::filesystem::exists(marker));
        auto error = std::error_code{};
        std::filesystem::remove(marker, error);
    }
};
