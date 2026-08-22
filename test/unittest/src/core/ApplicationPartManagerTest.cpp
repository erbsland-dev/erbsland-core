// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/ApplicationPartIdentifier.hpp>
#include <erbsland/core/ApplicationPartManager.hpp>
#include <erbsland/core/ApplicationPartTraits.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/Events.hpp>
#include <erbsland/event/UnmanagedEventThread.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionValues.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace el::core;
using namespace el::text::literals;

class ManagerTestRootInterface {
public:
    virtual ~ManagerTestRootInterface() = default;
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        static const auto identifier = ApplicationPartIdentifier::create("dev.erbsland.test.root"_el);
        return identifier;
    }
    [[nodiscard]] virtual auto workerThread() const noexcept -> std::thread::id = 0;
};

class ManagerTestDependentInterface {
public:
    virtual ~ManagerTestDependentInterface() = default;
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        static const auto identifier = ApplicationPartIdentifier::create("dev.erbsland.test.dependent"_el);
        return identifier;
    }
    [[nodiscard]] virtual auto workerThread() const noexcept -> std::thread::id = 0;
};

class ManagerTestRootPart final : public ApplicationPartWithInterface<ManagerTestRootInterface> {
public:
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestRootPart> {
        return std::make_shared<ManagerTestRootPart>();
    }
    [[nodiscard]] auto workerThread() const noexcept -> std::thread::id override { return _workerThread; }

protected:
    void initialize() override { _workerThread = std::this_thread::get_id(); }
    void running() override { cRunning = true; }
    void cleanup() noexcept override {
        cCleanupThread = std::this_thread::get_id();
        cLoopWasStoppedDuringCleanup = !std::dynamic_pointer_cast<el::event::EventLoop>(events())->isRunning();
        cRunning = false;
    }

public:
    inline static std::atomic<bool> cRunning{false};
    inline static std::atomic<bool> cLoopWasStoppedDuringCleanup{false};
    inline static std::thread::id cCleanupThread;

private:
    std::thread::id _workerThread;
};

class ManagerTestDependentPart final : public ApplicationPartWithInterface<ManagerTestDependentInterface> {
public:
    [[nodiscard]] static auto dependencies() -> ApplicationPartIdentifierList {
        return ApplicationPartIdentifierList{ManagerTestRootInterface::partIdentifier()};
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestDependentPart> {
        return std::make_shared<ManagerTestDependentPart>();
    }
    [[nodiscard]] auto workerThread() const noexcept -> std::thread::id override { return _workerThread; }

protected:
    [[nodiscard]] auto automaticStart() -> bool override {
        cDependencyWasRunning =
            partManager().partState(ManagerTestRootInterface::partIdentifier()) == ApplicationPartState::Running;
        return true;
    }
    void initialize() override { _workerThread = std::this_thread::get_id(); }

public:
    inline static std::atomic<bool> cDependencyWasRunning{false};

private:
    std::thread::id _workerThread;
};

class ManagerTestOptionalPart final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        static const auto identifier = ApplicationPartIdentifier::create("dev.erbsland.test.optional"_el);
        return identifier;
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestOptionalPart> {
        return std::make_shared<ManagerTestOptionalPart>();
    }

protected:
    [[nodiscard]] auto automaticStart() -> bool override { return false; }
};

class ManagerTestOptionalDependentPart final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        static const auto identifier = ApplicationPartIdentifier::create("dev.erbsland.test.optional-dependent"_el);
        return identifier;
    }
    [[nodiscard]] static auto dependencies() -> ApplicationPartIdentifierList {
        return ApplicationPartIdentifierList{ManagerTestOptionalPart::partIdentifier()};
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestOptionalDependentPart> {
        return std::make_shared<ManagerTestOptionalDependentPart>();
    }

protected:
    [[nodiscard]] auto automaticStart() -> bool override {
        ++cAutomaticStartCount;
        return true;
    }

public:
    inline static std::atomic<int> cAutomaticStartCount{0};
};

class ManagerTestFailurePart final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        static const auto identifier = ApplicationPartIdentifier::create("dev.erbsland.test.failure"_el);
        return identifier;
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestFailurePart> {
        return std::make_shared<ManagerTestFailurePart>();
    }

public:
    void fail() {
        events()->invoke([]() -> void { throw el::err::LogicError{"expected failure"_el}; });
    }
};

class ManagerTestTimeoutPart final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        static const auto identifier = ApplicationPartIdentifier::create("dev.erbsland.test.timeout"_el);
        return identifier;
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestTimeoutPart> {
        return std::make_shared<ManagerTestTimeoutPart>();
    }

protected:
    [[nodiscard]] auto shutdownTimeout() const noexcept -> el::time::TimeDelta override {
        return el::time::TimeDelta::milliseconds(5);
    }
    void stopping() override {}
};

class ManagerTestMissingPart final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.missing-owner"_el);
    }
    [[nodiscard]] static auto dependencies() -> ApplicationPartIdentifierList {
        return ApplicationPartIdentifierList{ApplicationPartIdentifier::create("dev.erbsland.test.not-registered"_el)};
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestMissingPart> {
        return std::make_shared<ManagerTestMissingPart>();
    }
};

class ManagerTestDuplicatePart final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.root"_el);
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestDuplicatePart> {
        return std::make_shared<ManagerTestDuplicatePart>();
    }
};

class ManagerTestWrongInterface {
public:
    virtual ~ManagerTestWrongInterface() = default;
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.root"_el);
    }
    virtual void wrongOperation() = 0;
};

class ManagerTestDeferredPart final : public ApplicationPart {
public:
    ManagerTestDeferredPart() { ++cConstructionCount; }
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.deferred"_el);
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestDeferredPart> {
        return std::make_shared<ManagerTestDeferredPart>();
    }

public:
    inline static std::atomic<int> cConstructionCount{0};
};

class ManagerTestNullFactoryPart final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.null-factory"_el);
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestNullFactoryPart> { return {}; }
};

class ManagerTestCyclePartA final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.cycle-a"_el);
    }
    [[nodiscard]] static auto dependencies() -> ApplicationPartIdentifierList {
        return ApplicationPartIdentifierList{ApplicationPartIdentifier::create("dev.erbsland.test.cycle-b"_el)};
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestCyclePartA> {
        return std::make_shared<ManagerTestCyclePartA>();
    }
};

class ManagerTestCyclePartB final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.cycle-b"_el);
    }
    [[nodiscard]] static auto dependencies() -> ApplicationPartIdentifierList {
        return ApplicationPartIdentifierList{ManagerTestCyclePartA::partIdentifier()};
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestCyclePartB> {
        return std::make_shared<ManagerTestCyclePartB>();
    }
};

class ManagerTestWaitPart final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.wait"_el);
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestWaitPart> {
        return std::make_shared<ManagerTestWaitPart>();
    }

protected:
    void running() override {
        try {
            static_cast<void>(partManager().waitForRunning());
        } catch (const el::err::LogicError &) {
            cWaitRejected = true;
        }
    }

public:
    inline static std::atomic<bool> cWaitRejected{false};
};

class ManagerTestLifetimeState final {
public:
    void blockCallback(const bool soleManagerOwner = true) {
        auto lock = std::unique_lock{_mutex};
        _callbackEntered = true;
        _soleManagerOwner = soleManagerOwner;
        _condition.notify_all();
        _condition.wait(lock, [this]() -> bool { return _releaseCallback; });
    }
    [[nodiscard]] auto waitForCallbackEntered() -> bool {
        auto lock = std::unique_lock{_mutex};
        return _condition.wait_for(lock, std::chrono::seconds{2}, [this]() -> bool { return _callbackEntered; });
    }
    [[nodiscard]] auto soleManagerOwner() const -> bool {
        const auto lock = std::scoped_lock{_mutex};
        return _soleManagerOwner;
    }
    void releaseCallback() {
        {
            const auto lock = std::scoped_lock{_mutex};
            _releaseCallback = true;
        }
        _condition.notify_all();
    }
    void markPartDestroyed() {
        {
            const auto lock = std::scoped_lock{_mutex};
            _partDestroyed = true;
        }
        _condition.notify_all();
    }
    [[nodiscard]] auto waitForPartDestroyed() -> bool {
        auto lock = std::unique_lock{_mutex};
        return _condition.wait_for(lock, std::chrono::seconds{2}, [this]() -> bool { return _partDestroyed; });
    }

private:
    mutable std::mutex _mutex;
    std::condition_variable _condition;
    bool _callbackEntered{false};
    bool _soleManagerOwner{false};
    bool _releaseCallback{false};
    bool _partDestroyed{false};
};

class ManagerTestLifetimePart final : public ApplicationPart {
public:
    ~ManagerTestLifetimePart() override {
        if (const auto state = cState.lock(); state != nullptr) {
            state->markPartDestroyed();
        }
    }

    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.lifetime"_el);
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestLifetimePart> {
        return std::make_shared<ManagerTestLifetimePart>();
    }

public:
    void releaseManagerFromPartThread(
        ApplicationPartManagerPtr manager, const std::shared_ptr<ManagerTestLifetimeState> &state) {
        events()->invoke([manager = std::move(manager), state]() mutable -> void {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
            while (manager.use_count() != 1 && std::chrono::steady_clock::now() < deadline) {
                std::this_thread::yield();
            }
            state->blockCallback(manager.use_count() == 1);
            manager.reset();
        });
    }

public:
    inline static std::weak_ptr<ManagerTestLifetimeState> cState;
};

class ManagerTestOptionPartOne final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.option-one"_el);
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestOptionPartOne> {
        return std::make_shared<ManagerTestOptionPartOne>();
    }

protected:
    void registerCommandLineOptions(const el::options::OptionsPtr &options) override {
        cOptions = options;
        cCalls.emplace_back(1);
    }
    void parseCommandLine(const el::options::OptionValuesPtr &values) override {
        cValues = values;
        cCalls.emplace_back(3);
    }

public:
    inline static std::vector<int> cCalls;
    inline static el::options::OptionsPtr cOptions;
    inline static el::options::OptionValuesPtr cValues;
};

class ManagerTestOptionPartTwo final : public ApplicationPart {
public:
    [[nodiscard]] static auto partIdentifier() -> ApplicationPartIdentifierPtr {
        return ApplicationPartIdentifier::create("dev.erbsland.test.option-two"_el);
    }
    [[nodiscard]] static auto create() -> std::shared_ptr<ManagerTestOptionPartTwo> {
        return std::make_shared<ManagerTestOptionPartTwo>();
    }

protected:
    void registerCommandLineOptions(const el::options::OptionsPtr &) override {
        ManagerTestOptionPartOne::cCalls.emplace_back(2);
    }
    void parseCommandLine(const el::options::OptionValuesPtr &) override {
        ManagerTestOptionPartOne::cCalls.emplace_back(4);
    }
};

TESTED_TARGETS(
    ApplicationPart ApplicationPartIdentifier ApplicationPartManager ApplicationPartManagerAccess
        ApplicationPartWithInterface)
class ApplicationPartManagerTest final : public el::UnitTest {
public:
    void testIdentifierValidation() {
        const auto identifier = ApplicationPartIdentifier::create("com.example.frontend-v2"_el);
        REQUIRE_EQUAL(identifier->name(), "com.example.frontend-v2"_el);
        REQUIRE_EQUAL(identifier->toString(), identifier->name());
        REQUIRE_THROWS_AS(el::err::ParameterError, ApplicationPartIdentifier::create({}));
        REQUIRE_THROWS_AS(el::err::ParameterError, ApplicationPartIdentifier::create("invalid/name"_el));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            ApplicationPartIdentifier::create(
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_el));
    }

    void testPrepareAndLookupByEquivalentIdentifier() {
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestRootPart>();
        manager->prepare();

        const auto equivalent = ApplicationPartIdentifier::create("dev.erbsland.test.root"_el);
        REQUIRE_EQUAL(
            manager->part(equivalent),
            std::dynamic_pointer_cast<ApplicationPart>(manager->part<ManagerTestRootInterface>()));
        REQUIRE_EQUAL(manager->partState(equivalent), ApplicationPartState::Uninitialized);

        const auto secondManager = ApplicationPartManager::create();
        secondManager->registerPart<ManagerTestRootPart>();
        secondManager->prepare();
        REQUIRE(secondManager->part(equivalent));
    }

    void testPreparationRejectsInvalidGraphs() {
        {
            const auto manager = ApplicationPartManager::create();
            manager->registerPart<ManagerTestRootPart>();
            manager->registerPart<ManagerTestDuplicatePart>();
            REQUIRE_THROWS_AS(el::err::LogicError, manager->prepare());
        }
        {
            const auto manager = ApplicationPartManager::create();
            manager->registerPart<ManagerTestMissingPart>();
            REQUIRE_THROWS_AS(el::err::LogicError, manager->prepare());
        }
        {
            const auto manager = ApplicationPartManager::create();
            manager->registerPart<ManagerTestCyclePartA>();
            manager->registerPart<ManagerTestCyclePartB>();
            REQUIRE_THROWS_AS(el::err::LogicError, manager->prepare());
        }
        {
            const auto manager = ApplicationPartManager::create();
            manager->registerPart<ManagerTestNullFactoryPart>();
            REQUIRE_THROWS_AS(el::err::LogicError, manager->prepare());
        }
    }

    void testConstructionIsDeferredAndRegistrationFreezes() {
        ManagerTestDeferredPart::cConstructionCount = 0;
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestDeferredPart>();
        REQUIRE_EQUAL(ManagerTestDeferredPart::cConstructionCount.load(), 0);
        manager->prepare();
        REQUIRE_EQUAL(ManagerTestDeferredPart::cConstructionCount.load(), 1);
        REQUIRE_THROWS_AS(el::err::LogicError, manager->registerPart<ManagerTestRootPart>());
        REQUIRE_THROWS_AS(el::err::LogicError, manager->part<ManagerTestWrongInterface>());
    }

    void testDependencyStartupAndReverseShutdown() {
        ManagerTestRootPart::cRunning = false;
        ManagerTestRootPart::cLoopWasStoppedDuringCleanup = false;
        ManagerTestRootPart::cCleanupThread = {};
        ManagerTestDependentPart::cDependencyWasRunning = false;
        auto callbackMutex = std::mutex{};
        auto callbackCondition = std::condition_variable{};
        auto runningCallbackObserved = false;
        auto controlThread = std::thread::id{};
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestDependentPart>();
        manager->registerPart<ManagerTestRootPart>();
        manager->setPartStateChangedFn(
            [&](ApplicationPartIdentifierPtr identifier, const ApplicationPartState state) -> void {
                if (identifier->name() == ManagerTestRootInterface::partIdentifier()->name() &&
                    state == ApplicationPartState::Running) {
                    {
                        const auto lock = std::scoped_lock{callbackMutex};
                        controlThread = std::this_thread::get_id();
                        runningCallbackObserved = true;
                    }
                    callbackCondition.notify_all();
                }
            });
        manager->prepare();
        manager->start();

        REQUIRE(manager->waitForRunning());
        REQUIRE(ManagerTestDependentPart::cDependencyWasRunning.load());
        const auto root = manager->part<ManagerTestRootInterface>();
        const auto dependent = manager->part<ManagerTestDependentInterface>();
        REQUIRE_NOT_EQUAL(root->workerThread(), std::thread::id{});
        REQUIRE_NOT_EQUAL(dependent->workerThread(), std::thread::id{});
        REQUIRE_NOT_EQUAL(root->workerThread(), dependent->workerThread());
        {
            auto lock = std::unique_lock{callbackMutex};
            callbackCondition.wait(lock, [&runningCallbackObserved]() -> bool { return runningCallbackObserved; });
        }
        REQUIRE_NOT_EQUAL(controlThread, root->workerThread());
        REQUIRE_NOT_EQUAL(controlThread, dependent->workerThread());

        manager->stop(ManagerTestRootInterface::partIdentifier());
        REQUIRE(manager->waitForStopped(ManagerTestDependentInterface::partIdentifier()));
        REQUIRE(manager->waitForStopped(ManagerTestRootInterface::partIdentifier()));
        REQUIRE_EQUAL(manager->state(), ApplicationPartManagerState::Running);
        REQUIRE_THROWS_AS(el::err::LogicError, manager->start(ManagerTestRootInterface::partIdentifier()));
        manager->stop();
        REQUIRE(manager->waitForStopped());
        REQUIRE_FALSE(ManagerTestRootPart::cRunning.load());
        REQUIRE(ManagerTestRootPart::cLoopWasStoppedDuringCleanup.load());
        REQUIRE_EQUAL(ManagerTestRootPart::cCleanupThread, root->workerThread());
    }

    void testManualStartRecursivelyStartsOptedOutDependency() {
        ManagerTestOptionalDependentPart::cAutomaticStartCount = 0;
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestOptionalDependentPart>();
        manager->registerPart<ManagerTestOptionalPart>();
        manager->prepare();
        manager->start();
        REQUIRE(manager->waitForRunning());
        REQUIRE_EQUAL(
            manager->partState(ManagerTestOptionalPart::partIdentifier()), ApplicationPartState::Uninitialized);
        REQUIRE_EQUAL(
            manager->partState(ManagerTestOptionalDependentPart::partIdentifier()),
            ApplicationPartState::Uninitialized);
        REQUIRE_EQUAL(ManagerTestOptionalDependentPart::cAutomaticStartCount.load(), 0);

        manager->start(ManagerTestOptionalDependentPart::partIdentifier());
        REQUIRE(manager->waitForRunning(ManagerTestOptionalDependentPart::partIdentifier()));
        REQUIRE_EQUAL(manager->partState(ManagerTestOptionalPart::partIdentifier()), ApplicationPartState::Running);
        REQUIRE_EQUAL(ManagerTestOptionalDependentPart::cAutomaticStartCount.load(), 0);

        manager->stop();
        REQUIRE(manager->waitForStopped());
    }

    void testContinueAfterPartFailure() {
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestRootPart>();
        manager->registerPart<ManagerTestFailurePart>();
        manager->setErrorHandler([](ApplicationPartIdentifierPtr, std::exception_ptr) -> ApplicationPartErrorAction {
            return ApplicationPartErrorAction::Continue;
        });
        manager->prepare();
        manager->start();
        REQUIRE(manager->waitForRunning());

        const auto failurePart =
            std::dynamic_pointer_cast<ManagerTestFailurePart>(manager->part(ManagerTestFailurePart::partIdentifier()));
        failurePart->fail();
        REQUIRE_FALSE(manager->waitForStopped(ManagerTestFailurePart::partIdentifier()));
        REQUIRE_EQUAL(manager->state(), ApplicationPartManagerState::Running);
        REQUIRE_EQUAL(manager->partState(ManagerTestRootPart::partIdentifier()), ApplicationPartState::Running);
        REQUIRE(manager->hasError());

        manager->stop();
        REQUIRE(manager->waitForStopped());
    }

    void testShutdownTimeoutFailsPart() {
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestTimeoutPart>();
        manager->setErrorHandler([](ApplicationPartIdentifierPtr, std::exception_ptr) -> ApplicationPartErrorAction {
            return ApplicationPartErrorAction::Continue;
        });
        manager->prepare();
        manager->start();
        REQUIRE(manager->waitForRunning());

        manager->stop(ManagerTestTimeoutPart::partIdentifier());
        REQUIRE_FALSE(manager->waitForStopped(ManagerTestTimeoutPart::partIdentifier()));
        REQUIRE(manager->hasError());

        manager->stop();
        REQUIRE(manager->waitForStopped());
    }

    void testWaitFromPartThreadIsRejected() {
        ManagerTestWaitPart::cWaitRejected = false;
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestWaitPart>();
        manager->prepare();
        manager->start();
        REQUIRE(manager->waitForRunning());
        REQUIRE(ManagerTestWaitPart::cWaitRejected.load());
        manager->stop();
        REQUIRE(manager->waitForStopped());
    }

    void testCommandLineForwardingUsesRegistrationOrder() {
        ManagerTestOptionPartOne::cCalls.clear();
        ManagerTestOptionPartOne::cOptions.reset();
        ManagerTestOptionPartOne::cValues.reset();
        const auto options = el::options::Options::create();
        const auto values = el::options::OptionValues::create();
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestOptionPartOne>();
        manager->registerPart<ManagerTestOptionPartTwo>();
        manager->prepare();

        manager->registerCommandLineOptions(options);
        manager->parseCommandLine(values);

        REQUIRE_EQUAL(ManagerTestOptionPartOne::cCalls, std::vector<int>({1, 2, 3, 4}));
        REQUIRE_EQUAL(ManagerTestOptionPartOne::cOptions, options);
        REQUIRE_EQUAL(ManagerTestOptionPartOne::cValues, values);
        manager->stop();
        REQUIRE(manager->waitForStopped());
    }

    void testInjectedControlThreadRemainsCallerOwned() {
        const auto controlThread = el::event::UnmanagedEventThread::create();
        controlThread->start();
        {
            const auto manager = ApplicationPartManager::create(controlThread->events());
            manager->registerPart<ManagerTestRootPart>();
            manager->prepare();
            manager->start();
            REQUIRE(manager->waitForRunning());
            manager->stop();
            REQUIRE(manager->waitForStopped());
        }
        REQUIRE(controlThread->isRunning());
        controlThread->quit();
        controlThread->join();
    }

    void testFinalControlThreadReferenceDefersDestruction() {
        const auto lifetimeState = std::make_shared<ManagerTestLifetimeState>();
        ManagerTestLifetimePart::cState = lifetimeState;
        auto manager = ApplicationPartManager::create();
        const auto managerWeak = std::weak_ptr<ApplicationPartManager>{manager};
        manager->registerPart<ManagerTestLifetimePart>();
        manager->setStateChangedFn([lifetimeState](const ApplicationPartManagerState state) -> void {
            if (state == ApplicationPartManagerState::Stopped) {
                lifetimeState->blockCallback();
            }
        });
        manager->prepare();
        manager->start();
        REQUIRE(manager->waitForRunning());
        manager->stop();
        REQUIRE(manager->waitForStopped());

        const auto callbackEntered = lifetimeState->waitForCallbackEntered();
        manager.reset();
        const auto remainingReferences = managerWeak.use_count();
        lifetimeState->releaseCallback();
        const auto partDestroyed = lifetimeState->waitForPartDestroyed();
        ManagerTestLifetimePart::cState.reset();

        REQUIRE(callbackEntered);
        REQUIRE_EQUAL(remainingReferences, 1);
        REQUIRE(partDestroyed);
        REQUIRE(managerWeak.expired());
    }

    void testFinalPartThreadReferenceDefersDestruction() {
        const auto lifetimeState = std::make_shared<ManagerTestLifetimeState>();
        ManagerTestLifetimePart::cState = lifetimeState;
        auto manager = ApplicationPartManager::create();
        const auto managerWeak = std::weak_ptr<ApplicationPartManager>{manager};
        manager->registerPart<ManagerTestLifetimePart>();
        manager->prepare();
        manager->start();
        REQUIRE(manager->waitForRunning());

        auto part = std::dynamic_pointer_cast<ManagerTestLifetimePart>(
            manager->part(ManagerTestLifetimePart::partIdentifier()));
        part->releaseManagerFromPartThread(manager, lifetimeState);
        part.reset();
        manager.reset();
        const auto callbackEntered = lifetimeState->waitForCallbackEntered();
        const auto soleManagerOwner = lifetimeState->soleManagerOwner();
        lifetimeState->releaseCallback();
        const auto partDestroyed = lifetimeState->waitForPartDestroyed();
        ManagerTestLifetimePart::cState.reset();

        REQUIRE(callbackEntered);
        REQUIRE(soleManagerOwner);
        REQUIRE(partDestroyed);
        REQUIRE(managerWeak.expired());
    }

    void testThrowingErrorHandlerQueuesErrorAfterPartError() {
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestFailurePart>();
        manager->setErrorHandler([](ApplicationPartIdentifierPtr, std::exception_ptr) -> ApplicationPartErrorAction {
            throw el::err::RuntimeError{"expected error-handler failure"_el};
        });
        manager->prepare();
        manager->start();
        REQUIRE(manager->waitForRunning());

        const auto part =
            std::dynamic_pointer_cast<ManagerTestFailurePart>(manager->part(ManagerTestFailurePart::partIdentifier()));
        part->fail();
        REQUIRE_FALSE(manager->waitForStopped());
        const auto partError = manager->takeError();
        const auto callbackError = manager->takeError();
        REQUIRE_THROWS_AS(el::err::LogicError, std::rethrow_exception(partError));
        REQUIRE_THROWS_AS(el::err::RuntimeError, std::rethrow_exception(callbackError));
        REQUIRE_FALSE(manager->hasError());
        REQUIRE_EQUAL(manager->state(), ApplicationPartManagerState::Failed);
    }

    void testThrowingStateCallbackForcesStopAll() {
        const auto manager = ApplicationPartManager::create();
        manager->registerPart<ManagerTestRootPart>();
        manager->setPartStateChangedFn([](ApplicationPartIdentifierPtr, const ApplicationPartState state) -> void {
            if (state == ApplicationPartState::Running) {
                throw el::err::RuntimeError{"expected state callback failure"_el};
            }
        });
        manager->prepare();
        manager->start();
        REQUIRE_FALSE(manager->waitForStopped());
        REQUIRE(manager->hasError());
        REQUIRE_THROWS_AS(el::err::RuntimeError, std::rethrow_exception(manager->takeError()));
    }
};
