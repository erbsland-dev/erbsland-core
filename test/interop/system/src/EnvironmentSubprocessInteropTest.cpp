// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Definitions.hpp>
#include <erbsland/DetachedSubprocess.hpp>
#include <erbsland/EnvironmentChanges.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/StdFormat.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <filesystem>
#include <thread>

using namespace el::text::literals;

TESTED_TARGETS(EnvironmentChanges SubprocessOptions Subprocess DetachedSubprocess)
class EnvironmentSubprocessInteropTest final : public el::UnitTest {
    using Changes = el::system::EnvironmentChanges;
    using Options = el::system::SubprocessOptions;
    using Path = el::path::Path;

    void setParent(const el::text::String &name, const el::text::String &value) {
        auto environment = el::system::EnvironmentVariables{};
        if (!_saved.contains(name)) {
            _saved.set(name, environment.get(name));
        }
        environment.setOrThrow(name, value);
    }

    static auto executable() -> Path { return Path{el::unittest::fh::unitTestExecutablePath()}; }

    static auto options() -> Options {
        return Options{}
            .setInheritStandardInput(false)
            .setStandardOutputMode(el::system::SubprocessOutputMode::Capture)
            .setStandardErrorMode(el::system::SubprocessOutputMode::Capture);
    }

    auto readChild(const el::text::String &name, const Options &options) -> el::text::String {
        auto child = el::system::Subprocess::create(
            executable(), el::text::StringList{"--subprocess-helper"_el, "environment"_el, name}, options);
        child->start();
        REQUIRE(child->wait().isSuccess());
        return child->standardOutput();
    }

public:
    void tearDown() override {
        auto environment = el::system::EnvironmentVariables{};
        for (const auto &[name, value] : _saved) {
            if (value.has_value()) {
                environment.setOrThrow(name, *value);
            } else {
                environment.removeOrThrow(name);
            }
        }
        _saved.clear();
        if (!_marker.empty()) {
            auto error = std::error_code{};
            std::filesystem::remove(_marker, error);
            _marker.clear();
        }
    }

    void testSnapshotAtStartAndCopiedOptions() {
        setParent("ERBSLAND_ENV_LATE"_el, "before"_el);
        auto config = options();
        config.environmentChanges().setInherit(el::text::StringList{"ERBSLAND_ENV_LATE"_el});
        auto child = el::system::Subprocess::create(
            executable(),
            el::text::StringList{"--subprocess-helper"_el, "environment"_el, "ERBSLAND_ENV_LATE"_el},
            config);
        config.environmentChanges().setVariable("ERBSLAND_ENV_LATE"_el, "wrong"_el);
        setParent("ERBSLAND_ENV_LATE"_el, "after"_el);
        child->start();
        REQUIRE(child->wait().isSuccess());
        REQUIRE_EQUAL(child->standardOutput(), "after"_el);
    }

    void testSelectedInheritanceExcludesOtherVariables() {
        setParent("ERBSLAND_ENV_SELECTED"_el, "selected"_el);
        setParent("ERBSLAND_ENV_EXCLUDED"_el, "excluded"_el);
        auto config = options().setEnvironmentChanges(
            Changes{}.setInherit(el::text::StringList{"ERBSLAND_ENV_SELECTED"_el, "ERBSLAND_ENV_ABSENT"_el}));
        REQUIRE_EQUAL(readChild("ERBSLAND_ENV_EXCLUDED"_el, config), "<missing>"_el);
    }

    void testVariableRemovalAndEmptyValues() {
        setParent("ERBSLAND_ENV_REMOVED"_el, "inherited"_el);
        auto config = options();
        config.environmentChanges().removeVariable("ERBSLAND_ENV_REMOVED"_el);
        REQUIRE_EQUAL(readChild("ERBSLAND_ENV_REMOVED"_el, config), "<missing>"_el);
    }

    void testEmptyVariableValue() {
        auto config = options();
        config.environmentChanges().setVariable("ERBSLAND_ENV_EMPTY"_el, ""_el);
        REQUIRE(readChild("ERBSLAND_ENV_EMPTY"_el, config).isEmpty());
    }

    void testPathProcessingAtLaunch() {
#ifdef ERBSLAND_OS_WINDOWS
        const auto separator = el::text::String{";"_el};
#else
        const auto separator = el::text::String{":"_el};
#endif
        const auto base =
            el::text::StringList{"keep"_el, "gone"_el, "sdk"_el, "sdk/bin"_el, "keep/."_el}.join(separator);
        auto config = options();
        config.environmentChanges()
            .setInherit(el::system::EnvironmentInherit::None)
            .setVariable("PATH"_el, base)
            .removePath(Path{"gone"_el})
            .removePathsIn(Path{"sdk"_el})
            .prependPath(Path{"front"_el})
            .appendPath(Path{"back"_el});
        const auto expected = el::text::StringList{"front"_el, "keep"_el, "back"_el}.join(separator);
        REQUIRE_EQUAL(readChild("PATH"_el, config), expected);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testDetachedEnvironmentAndPaths() {
        setParent("ERBSLAND_ENV_DETACHED"_el, "detached"_el);
        _marker = std::filesystem::temp_directory_path() /
            std::filesystem::path{el::text::StringConverter{
                el::join(
                    "erbsland-environment-"_el,
                    el::text::String::fromInteger(std::chrono::steady_clock::now().time_since_epoch().count()))}
                    .toStdString()};
        auto config = Options{}
                          .setInheritStandardInput(false)
                          .setStandardOutputMode(el::system::SubprocessOutputMode::Discard)
                          .setStandardErrorMode(el::system::SubprocessOutputMode::Discard);
        config.environmentChanges()
            .setInherit(el::text::StringList{"ERBSLAND_ENV_DETACHED"_el})
            .prependPath(Path{"front"_el});
        const auto child = el::system::DetachedSubprocess::start(
            executable(),
            el::text::StringList{
                "--subprocess-helper"_el,
                "environment-marker"_el,
                Path{_marker}.toString(),
                "ERBSLAND_ENV_DETACHED"_el},
            config);
        REQUIRE(child != nullptr);
        const auto content = Path{_marker}.content();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{3};
        auto text = content.readText();
        while (
            text != std::optional<el::text::String>{"detached\nfront"_el} &&
            std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
            text = content.readText();
        }
        REQUIRE(text.has_value());
        REQUIRE_EQUAL(*text, "detached\nfront"_el);
    }

private:
    el::text::StringMap<std::optional<el::text::String>> _saved;
    std::filesystem::path _marker;
};
