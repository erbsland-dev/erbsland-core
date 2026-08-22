// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/core/ApplicationPart.hpp>
#include <erbsland/core/ApplicationPartIdentifier.hpp>
#include <erbsland/core/ApplicationPartManager.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/options/OptionModule.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionValues_fwd.hpp>
#include <erbsland/stream/AnyStringBuilderStream.hpp>
#include <erbsland/stream/StandardStreams.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/ExitCode.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <memory>

using namespace el::core;
using namespace el::text::literals;

class ApplicationIntegratedPart final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        static const auto identifier = ApplicationPartIdentifier::create("dev.erbsland.test.application-part"_el);
        return identifier;
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ApplicationIntegratedPart> {
        return std::make_shared<ApplicationIntegratedPart>();
    }

public:
    static void reset() noexcept {
        cRegisterCount = 0;
        cParseCount = 0;
        cInitializeCount = 0;
        cCleanupCount = 0;
        cQuitOnRunning = false;
        cFailOnInitialize = false;
    }

protected:
    void registerCommandLineOptions(const el::options::OptionsPtr &options) override {
        if (options != nullptr) {
            ++cRegisterCount;
        }
    }
    void parseCommandLine(const el::options::OptionValuesPtr &values) override {
        if (values != nullptr) {
            ++cParseCount;
        }
    }
    void initialize() override {
        ++cInitializeCount;
        if (cFailOnInitialize) {
            throw el::err::RuntimeError{"expected application-part failure"_el};
        }
    }
    void running() override {
        if (cQuitOnRunning) {
            application().quit(el::unit::ExitCode{17});
            application().quit(el::unit::ExitCode{23});
        }
    }
    void cleanup() noexcept override { ++cCleanupCount; }

public:
    inline static std::atomic<int> cRegisterCount{0};
    inline static std::atomic<int> cParseCount{0};
    inline static std::atomic<int> cInitializeCount{0};
    inline static std::atomic<int> cCleanupCount{0};
    inline static std::atomic<bool> cQuitOnRunning{false};
    inline static std::atomic<bool> cFailOnInitialize{false};
};

class ApplicationPartOrderApplication final : public Application {
protected:
    void cleanup() noexcept override {
        cPartWasCleanBeforeApplication = ApplicationIntegratedPart::cCleanupCount.load() == 1;
    }

public:
    inline static std::atomic<bool> cPartWasCleanBeforeApplication{false};
};

class ApplicationPartCustomMainApplication final : public Application {
protected:
    [[nodiscard]] auto main() -> el::unit::ExitCode override { return el::unit::ExitCode{31}; }
};

TESTED_TARGETS(Application ApplicationPart ApplicationPartManager)
class ApplicationPartApplicationTest final : public el::UnitTest {
public:
    void testDefaultMainStartsAndCoordinatesPartShutdown() {
        ApplicationIntegratedPart::reset();
        ApplicationIntegratedPart::cQuitOnRunning = true;
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        app.registerPart<ApplicationIntegratedPart>();

        REQUIRE_EQUAL(app.run(), 17);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cRegisterCount.load(), 1);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cParseCount.load(), 1);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cInitializeCount.load(), 1);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cCleanupCount.load(), 1);
        REQUIRE_EQUAL(app.partManager()->state(), ApplicationPartManagerState::Stopped);
    }

    void testPartCleanupPrecedesApplicationCleanup() {
        ApplicationIntegratedPart::reset();
        ApplicationIntegratedPart::cQuitOnRunning = true;
        ApplicationPartOrderApplication::cPartWasCleanBeforeApplication = false;
        auto scope = ApplicationTestScope<ApplicationPartOrderApplication>{};
        auto &app = scope.app();
        app.registerPart<ApplicationIntegratedPart>();

        REQUIRE_EQUAL(app.run(), 17);
        REQUIRE(ApplicationPartOrderApplication::cPartWasCleanBeforeApplication.load());
    }

    void testCustomMainDoesNotAutomaticallyStartParts() {
        ApplicationIntegratedPart::reset();
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        app.registerPart<ApplicationIntegratedPart>();
        app.setMainFn([]() -> el::unit::ExitCode { return el::unit::ExitCode{23}; });

        REQUIRE_EQUAL(app.run(), 23);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cRegisterCount.load(), 1);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cParseCount.load(), 1);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cInitializeCount.load(), 0);
        REQUIRE_EQUAL(app.partManager()->state(), ApplicationPartManagerState::Stopped);
    }

    void testHelpSkipsPartParsingAndStartup() {
        ApplicationIntegratedPart::reset();
        char arg0[] = "tool";
        char arg1[] = "--help";
        char *argv[] = {arg0, arg1};
        auto scope = ApplicationTestScope<Application>{2, argv};
        auto &app = scope.app();
        app.registerPart<ApplicationIntegratedPart>();
        const auto output = el::stream::AnyStringBuilderStream::create();
        const auto redirect = el::stream::redirectStandardStreams(output, output);

        REQUIRE_EQUAL(app.run(), 0);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cRegisterCount.load(), 1);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cParseCount.load(), 0);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cInitializeCount.load(), 0);
    }

    void testModuleAndOverriddenMainsDoNotAutomaticallyStartParts() {
        ApplicationIntegratedPart::reset();
        char arg0[] = "tool";
        char arg1[] = "run";
        char *argv[] = {arg0, arg1};
        {
            auto scope = ApplicationTestScope<Application>{2, argv};
            auto &app = scope.app();
            app.registerPart<ApplicationIntegratedPart>();
            const auto module = el::options::OptionModule::create("run"_el);
            module->setMainFn(
                [](el::options::OptionValuesPtr) -> el::unit::ExitCode { return el::unit::ExitCode{29}; });
            app.options()->addModule(module);
            REQUIRE_EQUAL(app.run(), 29);
            REQUIRE_EQUAL(ApplicationIntegratedPart::cInitializeCount.load(), 0);
        }
        ApplicationIntegratedPart::reset();
        {
            auto scope = ApplicationTestScope<ApplicationPartCustomMainApplication>{};
            auto &app = scope.app();
            app.registerPart<ApplicationIntegratedPart>();
            REQUIRE_EQUAL(app.run(), 31);
            REQUIRE_EQUAL(ApplicationIntegratedPart::cInitializeCount.load(), 0);
        }
    }

    void testManagerFailurePropagatesThroughApplicationRun() {
        ApplicationIntegratedPart::reset();
        ApplicationIntegratedPart::cFailOnInitialize = true;
        auto scope = ApplicationTestScope<Application>{};
        auto &app = scope.app();
        app.registerPart<ApplicationIntegratedPart>();
        const auto output = el::stream::AnyStringBuilderStream::create();
        const auto redirect = el::stream::redirectStandardStreams(output, output);

        REQUIRE_EQUAL(app.run(), el::unit::ExitCode::failure().toRawValue());
        REQUIRE_EQUAL(ApplicationIntegratedPart::cInitializeCount.load(), 1);
        REQUIRE_EQUAL(ApplicationIntegratedPart::cCleanupCount.load(), 1);
        REQUIRE_EQUAL(app.partManager()->state(), ApplicationPartManagerState::Failed);
    }
};
