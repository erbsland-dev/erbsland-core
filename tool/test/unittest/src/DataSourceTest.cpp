// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include "impl/DataSourceProbeTool.hpp"
#include "impl/DataSourceTestState.hpp"
#include "impl/FirstDataSource.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionValue.hpp>
#include <tool/frame/DataSource.hpp>
#include <tool/psl/PublicSuffixTool.hpp>

using namespace el::text::literals;
using namespace tool::test::impl;

TESTED_TARGETS(DataSource Tool ToolApplication DataSourceRegistration DataSourceType)
class DataSourceTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
private:
    DataSourceTestState &_sourceState = DataSourceTestState::shared(); ///< Shared lifecycle observations.
    static constexpr auto cUnicodeValue = "unicode"_el;

private:
    /// Source exercising the base configuration defaults.
    class BasicSource : public tool::frame::DataSource {
    public: // implement DataSource
        void initialize() override { DataSourceTestState::shared().phase("basic.initialize"_el); }
        /// Get the fixture identifier.
        [[nodiscard]] static auto sourceIdentifier() -> el::String { return "basic"_el; }
    };
    /// Concrete source lacking the required static identity.
    class MissingIdentifierSource : public tool::frame::DataSource {
    public: // implement DataSource
        void initialize() override {}
    };
    /// Source providing an incompatible static identifier result.
    class IncorrectIdentifierSource : public FirstDataSource {
    public: // fixture identity
        /// Get an incompatible identifier result.
        [[nodiscard]] static auto sourceIdentifier() -> int { return 0; }
    };
    /// Source deliberately requiring constructor arguments.
    class NonDefaultSource : public FirstDataSource {
    public: // fixture construction
        /// Require an argument to disable default construction.
        explicit NonDefaultSource(int) {}
    };
    /// Second source with the same configuration behavior and a distinct identity.
    class SecondSource : public FirstDataSource {
    public: // fixture identity
        /// Get the fixture's registration identifier.
        [[nodiscard]] static auto sourceIdentifier() -> el::String { return "second"_el; }

    protected: // implement FirstDataSource
        [[nodiscard]] auto label() const -> el::String override { return "second"_el; }
    };
    /// Distinct type deliberately claiming the first source's identifier.
    class AliasSource : public FirstDataSource {};
    /// Source with an invalid identifier.
    class InvalidSource : public FirstDataSource {
    public: // fixture identity
        /// Get an invalid fixture identifier.
        [[nodiscard]] static auto sourceIdentifier() -> el::String { return "bad-source"_el; }
    };
    /// Source deliberately colliding with the rendering section.
    class ReservedSource : public FirstDataSource {
    public: // fixture identity
        /// Get the reserved fixture identifier.
        [[nodiscard]] static auto sourceIdentifier() -> el::String { return "rendering"_el; }
    };
    /// Source deliberately colliding with the fixture tool.
    class ToolCollisionSource : public FirstDataSource {
    public: // fixture identity
        /// Get the colliding fixture identifier.
        [[nodiscard]] static auto sourceIdentifier() -> el::String { return "probe"_el; }
    };
    /// Unregistered tool used to verify that late registration is rejected.
    class LateTool : public NoConfigurationTool {
    public: // implement Tool
        [[nodiscard]] auto identifier() const -> el::String override { return "late"_el; }
    };

public: // implement UnitTest
    void setUp() override {
        ProjectFixture::setUp();
        _sourceState.events = {};
        _sourceState.dependencies =
            el::StringList{FirstDataSource::sourceIdentifier(), SecondSource::sourceIdentifier()};
        _sourceState.failure = {};
        _sourceState.withRules = true;
        _sourceState.phaseAction = {};
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[first]\n[second]\n"_el);
    }
    void tearDown() override {
        _sourceState.phaseAction = {};
        ProjectFixture::tearDown();
    }

public: // tests
    void testSourceDeclarationsAndTraitsLight() {
        static_assert(tool::frame::impl::DataSourceType<FirstDataSource>);
        static_assert(tool::frame::impl::DataSourceType<SecondSource>);
        static_assert(!tool::frame::impl::DataSourceType<tool::frame::DataSource>);
        static_assert(!tool::frame::impl::DataSourceType<NoConfigurationTool>);
        static_assert(!tool::frame::impl::DataSourceType<MissingIdentifierSource>);
        static_assert(!tool::frame::impl::DataSourceType<IncorrectIdentifierSource>);
        static_assert(!tool::frame::impl::DataSourceType<NonDefaultSource>);
        REQUIRE_EQUAL(
            tool::idna::IdnaTool{}.usedDataSources(), el::StringList{tool::unicode::UnicodeSource::sourceIdentifier()});
        REQUIRE_EQUAL(
            tool::psl::PublicSuffixTool{}.usedDataSources(),
            (el::StringList{
                tool::unicode::UnicodeSource::sourceIdentifier(), tool::psl::PublicSuffixSource::sourceIdentifier()}));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRegistrationIdentityAndEarlyAccess() {
        auto application = tool::frame::ToolApplication{};
        auto source = FirstDataSource{};
        REQUIRE_THROWS(source.logging());
        REQUIRE_THROWS(application.source<FirstDataSource>());
        application.registerSource<FirstDataSource>();
        REQUIRE_THROWS(application.source<FirstDataSource>());
        REQUIRE_THROWS(application.registerSource<FirstDataSource>());
        REQUIRE_THROWS(application.registerSource<AliasSource>());
        REQUIRE_THROWS(application.registerSource<InvalidSource>());
        REQUIRE_THROWS(application.registerSource<ReservedSource>());
        application.registerTool<DataSourceProbeTool>();
        REQUIRE_THROWS(application.registerSource<ToolCollisionSource>());
        auto reverseOrder = tool::frame::ToolApplication{};
        reverseOrder.registerSource<ToolCollisionSource>();
        REQUIRE_THROWS(reverseOrder.registerTool<DataSourceProbeTool>());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testLifecycleDefaultsDeduplicationAndTypedAccess() {
        _sourceState.dependencies.append(FirstDataSource::sourceIdentifier());
        _sourceState.phaseAction = [&](const auto &name) {
            if (name == "first.initialize"_el) {
                REQUIRE_THROWS(tool::frame::app().source<FirstDataSource>());
            } else if (name == "second.initialize"_el) {
                REQUIRE_EQUAL(tool::frame::app().source<FirstDataSource>().value, 42);
                REQUIRE_THROWS(tool::frame::app().source<SecondSource>());
            }
        };
        REQUIRE_EQUAL(
            runProbe([&](const auto &) {
                auto &application = tool::frame::app();
                REQUIRE_EQUAL(application.source<FirstDataSource>().value, 42);
                REQUIRE_EQUAL(application.source<SecondSource>().value, 42);
                REQUIRE_EQUAL(&application.source<FirstDataSource>(), &application.source<FirstDataSource>());
                REQUIRE_THROWS(application.source<AliasSource>());
                REQUIRE_THROWS(application.source<tool::unicode::UnicodeSource>());
                REQUIRE_THROWS(application.registerSource<BasicSource>());
                REQUIRE_THROWS(application.registerTool<LateTool>());
            }),
            el::ExitCode::success().toRawValue());
        REQUIRE_EQUAL(
            _sourceState.events,
            (el::StringList{
                "tool.configure"_el,
                "first.configure"_el,
                "second.configure"_el,
                "first.initialize"_el,
                "second.initialize"_el,
                "tool.initialize"_el,
                "tool.execute"_el}));
        REQUIRE(_outputText.contains("Initialized source: first"_el));
        REQUIRE(_outputText.contains("Initialized source: second"_el));
    }

    void testTypedAccessRequiresTheRegisteredConcreteType() {
        REQUIRE_EQUAL(
            runTool<DataSourceProbeTool>(
                [&](const auto &) {
                    REQUIRE_THROWS(tool::frame::app().source<FirstDataSource>());
                    REQUIRE_EQUAL(tool::frame::app().source<AliasSource>().value, 42);
                },
                [](auto &application) {
                    application.template registerSource<AliasSource>();
                    application.template registerSource<SecondSource>();
                }),
            el::ExitCode::success().toRawValue());
    }

    void testConfigurationFailuresPrecedeLoadingLight() {
        _sourceState.failure = "tool.configure"_el;
        REQUIRE_EQUAL(runProbe(), el::ExitCode::failure().toRawValue());
        REQUIRE_EQUAL(_sourceState.events, el::StringList{"tool.configure"_el});
        _sourceState.events = {};
        _sourceState.failure = "second.configure"_el;
        REQUIRE_EQUAL(runProbe(), el::ExitCode::failure().toRawValue());
        REQUIRE_EQUAL(
            _sourceState.events, (el::StringList{"tool.configure"_el, "first.configure"_el, "second.configure"_el}));
    }

    void testInvalidOptionPrecedesSourceLoadingLight() {
        const auto option = el::OptionValue::create(el::String{"invalid"_el}, {el::ArgumentIndex{2}});
        _sourceState.phaseAction = [&](const auto &name) {
            if (name == "tool.configure"_el) {
                throw el::OptionError{"Invalid Test Option"_el, "Rejected during command configuration."_el, option};
            }
        };
        REQUIRE_EQUAL(runProbe(), el::ExitCode::failure().toRawValue());
        REQUIRE_EQUAL(_sourceState.events, el::StringList{"tool.configure"_el});
        REQUIRE(_errorText.contains("Invalid Test Option"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRuleConstructionFailuresPrecedeLoading() {
        for (const auto &name : el::StringList{"tool.rules"_el, "first.rules"_el, "second.rules"_el}) {
            _sourceState.events = {};
            _sourceState.failure = name;
            REQUIRE_EQUAL(runProbe(), el::ExitCode::failure().toRawValue());
            REQUIRE_EQUAL(_sourceState.events, el::StringList{name});
            REQUIRE(_errorText.contains(name));
        }
    }

    void testUnusedSourcesAndDeclarationOrder() {
        _sourceState.dependencies = el::StringList{SecondSource::sourceIdentifier()};
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[first]\nunknown: 1\n[second]\nvalue: 7\n"_el);
        REQUIRE_EQUAL(
            runProbe([&](const auto &) {
                REQUIRE_THROWS(tool::frame::app().source<FirstDataSource>());
                REQUIRE_EQUAL(tool::frame::app().source<SecondSource>().value, 7);
            }),
            el::ExitCode::success().toRawValue());
        REQUIRE_EQUAL(
            _sourceState.events,
            (el::StringList{
                "tool.configure"_el,
                "second.configure"_el,
                "second.initialize"_el,
                "tool.initialize"_el,
                "tool.execute"_el}));
    }

    void testBaseSourceConfigurationDefaults() {
        _sourceState.dependencies = el::StringList{BasicSource::sourceIdentifier()};
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n"_el);
        REQUIRE_EQUAL(
            runTool<DataSourceProbeTool>(
                [&](const auto &) { REQUIRE_FALSE(tool::frame::app().source<BasicSource>().configurationRules()); },
                [](auto &application) { application.template registerSource<BasicSource>(); }),
            el::ExitCode::success().toRawValue());
        REQUIRE_EQUAL(
            _sourceState.events,
            (el::StringList{"tool.configure"_el, "basic.initialize"_el, "tool.initialize"_el, "tool.execute"_el}));
    }

    void testConfigurationFreeSources() {
        _sourceState.withRules = false;
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n"_el);
        REQUIRE_EQUAL(
            runProbe([&](const auto &) {
                REQUIRE_FALSE(tool::frame::app().source<FirstDataSource>().hasConfiguration);
                REQUIRE_FALSE(tool::frame::app().source<SecondSource>().hasConfiguration);
            }),
            el::ExitCode::success().toRawValue());
        _sourceState.events = {};
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[first]\n"_el);
        REQUIRE_EQUAL(runProbe(), el::ExitCode::failure().toRawValue());
        REQUIRE(_sourceState.events.isEmpty());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAllDependenciesAndSectionsValidatedBeforeLoading() {
        for (
            const auto &configuration : el::StringList{
                "[rendering]\n[first]\n"_el,
                "[rendering]\n[first]\n[second]\nvalue: \"bad\"\n"_el,
                "[rendering]\n[first]\n[second]\nunknown: 1\n"_el,
                "[rendering]\n[first]\n*[second]\nvalue: 1\n"_el}) {
            write("tool/conf/erbsland-core-tool.elcl"_el, configuration);
            REQUIRE_EQUAL(runProbe(), el::ExitCode::failure().toRawValue());
            REQUIRE(_sourceState.events.isEmpty());
        }
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[first]\n[second]\n"_el);
        _sourceState.dependencies.append("missing"_el);
        REQUIRE_THROWS_AS(el::LogicFailure, runProbe());
        REQUIRE(_sourceState.events.isEmpty());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFailuresStopTheRemainingLifecycle() {
        const auto phases = el::StringList{
            "tool.configure"_el,
            "first.configure"_el,
            "second.configure"_el,
            "first.initialize"_el,
            "second.initialize"_el,
            "tool.initialize"_el,
            "tool.execute"_el};
        auto expected = el::StringList{};
        for (const auto &name : phases) {
            _sourceState.events = {};
            _sourceState.failure = name;
            expected.append(name);
            REQUIRE_EQUAL(runProbe(), el::ExitCode::failure().toRawValue());
            REQUIRE_EQUAL(_sourceState.events, expected);
            REQUIRE(_errorText.contains(name));
        }
    }

    void testConcreteSourceSchemasAndUninitializedGetters() {
        auto unicode = tool::unicode::UnicodeSource{};
        auto suffix = tool::psl::PublicSuffixSource{};
        REQUIRE_THROWS(unicode.dataset());
        REQUIRE_THROWS(unicode.unicodeData());
        REQUIRE_THROWS(unicode.idnaData());
        REQUIRE_THROWS(unicode.normalizer());
        REQUIRE_THROWS(unicode.mapping());
        REQUIRE_THROWS(suffix.text());
        REQUIRE_THROWS(suffix.version());
        auto parser = el::conf::Parser{};
        for (
            const auto &text : el::StringList{
                "[unicode]\n"_el,
                "[unicode]\nunicode_version: \"\"\n"_el,
                "[unicode]\nunicode_version: \"18.0.0\"\nunknown: 1\n"_el}) {
            REQUIRE_THROWS(
                unicode.configurationRules()->validate(parser.parseTextOrThrow(text)->value(cUnicodeValue), 1));
        }
        const auto section =
            parser.parseTextOrThrow("[unicode]\nunicode_version: \"18.0.0\"\n"_el)->value(cUnicodeValue);
        unicode.configurationRules()->validate(section, 1);
        REQUIRE_EQUAL(section->getTextOrThrow("ucd_directory"_el), "tool/data/ucd"_el);
        const auto pslSection = parser.parseTextOrThrow("[public_suffix]\n"_el)->value("public_suffix"_el);
        suffix.configurationRules()->validate(pslSection, 1);
        REQUIRE_EQUAL(pslSection->getTextOrThrow("input_file"_el), "tool/data/public_suffix/public_suffix_list.dat"_el);
    }

private:
    /// Run the probe with reverse registration order and declared source order.
    /// @param action Assertions to run from execute after successful initialization.
    /// @return The process exit code.
    [[nodiscard]] auto runProbe(const std::function<void(const el::conf::ValuePtr &)> &action = {}) -> int {
        return runTool<DataSourceProbeTool>(action, [](auto &application) {
            application.template registerSource<SecondSource>();
            application.template registerSource<FirstDataSource>();
        });
    }
};
