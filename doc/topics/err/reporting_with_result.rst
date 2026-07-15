..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Result; Error Reporting
    single: Error Handling; Result
    single: Result; Custom Types
    single: Result; Exceptions

********************************
Reporting Errors with ``Result``
********************************

:cpp:class:`Result <erbsland::util::Result>` reports a successful or failed operation as an ordinary return value.
This page explains why a named result is clearer than a Boolean value, shows readable ways to handle results, and guides
you through defining a custom result with several meaningful states.
You will also learn when an exception communicates failure more effectively.

Prefer Named Outcomes over ``bool``
===================================

A ``bool`` can represent two outcomes, but it does not explain what either value means.
At a call site such as ``if (prepareRobotArm())``, the reader has to remember whether ``true`` means that the operation
succeeded, that an error occurred, or merely that a condition was present.
Negation makes this uncertainty even more visible: ``if (!prepareRobotArm())`` is compact, but not self-explanatory.

``Result::Success`` and ``Result::Failure`` give both outcomes names.
The tests :cpp:func:`Result::isSuccessful() <erbsland::util::Result::isSuccessful>` and
:cpp:func:`Result::isFailure() <erbsland::util::Result::isFailure>` make the decision explicit at the call site.
The base result stores its state in one byte, and its design leaves room for a domain-specific result to introduce more
states without changing how callers test the broad success and failure groups.

.. erbsland-demo::
    :source: err/Result/ClearStatuses.cpp
    :exec: err/result --demo ClearStatuses
    :source-sha256: 1faaed73377ad18b617a439cc06d30bba6f8e54f1126adfbfc68e5ae48a16ec5

.. code-block:: cpp

    /// A `Result` gives both outcomes meaningful names at the function boundary and at the call site.
    /// Unlike a `bool`, the return type communicates that the value reports the outcome of an operation.
    [[nodiscard]] auto prepareRobotArm(const el::StringView &armName) noexcept -> el::Result {
        return armName == "Aurora"_el ? el::Result::Success : el::Result::Failure;
    }

    /// Callers can test the named result without having to remember what `true` or `false` means.
    void clearStatuses() {
        if (isSuccessful(prepareRobotArm("Aurora"_el))) {
            el::io::printLine("The Aurora arm is ready."_el);
        }

        if (isFailure(prepareRobotArm("Neblina"_el))) {
            el::io::printLine("The Neblina arm could not be prepared."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    The Aurora arm is ready.
    The Neblina arm could not be prepared.

.. erbsland-demo-end::

Use ``Result`` when both outcomes are part of the normal contract and the caller is expected to make the next decision.
A named result improves the interface even when the operation currently has only two states, because it documents the
meaning of the return value and can evolve into a derived result if callers later need another actionable outcome.

Handle Results at the Call Site
===============================

Choose the handling form based on what the current code needs to know.
All three forms below use the same success and failure grouping; they differ only in whether the exact result must
remain available after the initial test.

Test a Temporary with a Free Function
-------------------------------------

Use ``isSuccessful(method())`` or ``isFailure(method())`` when the branch only needs the broad outcome.
This reads naturally and avoids introducing a local variable that would not otherwise be used.
The free functions accept :cpp:class:`Result <erbsland::util::Result>` and derived result types, so the call remains
unchanged if an API returns a more specific result.

Keep the Result for Exact-State Handling
----------------------------------------

Use an ``if`` initializer when a custom result has several failure or success states and the branch must distinguish
them:

.. code-block:: cpp

    if (auto result = method(); isFailure(result)) {
        // Inspect the exact failure state here.
    }

The operation runs once, and ``result`` remains available throughout the branch.
Do not call the method once for the group test and again for the exact-state test: the second call may be expensive, may
have side effects, or may produce a different outcome.

Use the Member Form for a Compact Decision
------------------------------------------

The equivalent member forms, ``method().isSuccessful()`` and ``method().isFailure()``, are useful when they make the
expression read more naturally from left to right.
Like the free functions, the inherited member functions work with custom derived result types.

.. erbsland-demo::
    :source: err/Result/HandlingResults.cpp
    :exec: err/result --demo HandlingResults
    :source-sha256: 7f330d06b94d697a734480e1fe542f11ec790443cd1f026169a9725389b8c436

.. code-block:: cpp

    /// Results support free-function and member-function tests.
    /// The free functions accept derived result types, while an initializer keeps a multi-state result available for
    /// inspecting its exact value.
    void handlingResults() {
        // Test a temporary directly when only the success group matters.
        if (isSuccessful(calibrateModule("visão"_el))) {
            el::io::printLine("The vision module is calibrated."_el);
        }

        // Keep the result when the exact failure state determines the response.
        if (auto result = calibrateModule("garra"_el); isFailure(result)) {
            if (result == CalibrationResult::Obstructed) {
                el::io::printLine("The gripper is obstructed."_el);
            }
        }

        // The member form is equally readable for a single grouped decision.
        if (calibrateModule("movimento"_el).isSuccessful()) {
            el::io::printLine("The movement module is ready."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    The vision module is calibrated.
    The gripper is obstructed.
    The movement module is ready.

.. erbsland-demo-end::

Create a Custom Result Type
===========================

Derive from :cpp:class:`Result <erbsland::util::Result>` when one operation has a small set of named outcomes and the
caller can react differently to them.
Expose the protected result constructor with ``using Result::Result``, then define constants with
``Value::success<N>()`` and ``Value::failure<N>()``.
The number ``N`` identifies a state within its group; use a different number for every distinct state.

The group is part of the value itself.
Therefore, ``isSuccessful()`` accepts every value created with ``success<N>()``, while ``isFailure()`` accepts every
value created with ``failure<N>()``.
Callers can handle the common path with a group test and compare against a named constant only where the distinction
matters.

.. erbsland-demo::
    :source: err/Result/CustomResultTypes.cpp
    :exec: err/result --demo CustomResultTypes
    :source-sha256: 77d2989d5ec2e8342a20de6d85af8e56f04e22a5ef784ef5933a93d7060163d4

.. code-block:: cpp

    /// Derive a custom result when callers need a small set of distinct, actionable outcomes.
    /// Values created with `success<N>()` belong to the successful group, while `failure<N>()` values belong to the
    /// failure group.
    class RobotSetupResult final : public el::Result {
    public:
        using Result::Result;

    public:
        static const RobotSetupResult Ready;
        static const RobotSetupResult AlreadyReady;
        static const RobotSetupResult Obstructed;
        static const RobotSetupResult ControllerOffline;
    };

    inline constexpr RobotSetupResult RobotSetupResult::Ready = Value::success<0>();
    inline constexpr RobotSetupResult RobotSetupResult::AlreadyReady = Value::success<1>();
    inline constexpr RobotSetupResult RobotSetupResult::Obstructed = Value::failure<0>();
    inline constexpr RobotSetupResult RobotSetupResult::ControllerOffline = Value::failure<1>();

    [[nodiscard]] auto setUpRobot(const el::StringView &robotName) noexcept -> RobotSetupResult {
        if (robotName == "Lume"_el) {
            return RobotSetupResult::Ready;
        }
        if (robotName == "Brisa"_el) {
            return RobotSetupResult::AlreadyReady;
        }
        if (robotName == "Pedra"_el) {
            return RobotSetupResult::Obstructed;
        }
        return RobotSetupResult::ControllerOffline;
    }

    /// Group tests handle the common path, and equality tests distinguish the states that require different actions.
    void customResultTypes() {
        if (isSuccessful(setUpRobot("Brisa"_el))) {
            el::io::printLine("Brisa is ready for the project."_el);
        }

        if (auto result = setUpRobot("Pedra"_el); isFailure(result)) {
            if (result == RobotSetupResult::Obstructed) {
                el::io::printLine("Remove the obstacle from Pedra."_el);
            } else if (result == RobotSetupResult::ControllerOffline) {
                el::io::printLine("Connect the Pedra controller."_el);
            }
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Brisa is ready for the project.
    Remove the obstacle from Pedra.

.. erbsland-demo-end::

Keep the State Set Focused
--------------------------

Add a state only when it enables a distinct and useful caller action.
For example, an obstruction may ask the caller to clear a work area, while an offline controller may ask it to restore a
connection.
Several internal reasons that all lead to the same response should usually remain one public failure state.

Too many success or failure states force every caller to understand details that belong inside the operation.
If callers need rich context, nested causes, source locations, or a user-facing diagnostic, an exception is usually a
better transport.
If a successful operation primarily needs to return data, return that data through the API designed for it rather than
turning every possible value into another result state.

Choose Between ``Result`` and Exceptions
========================================

The choice depends on control flow and information needs, not on how severe the failure sounds.
Use the smallest mechanism that preserves what the caller must know.

.. list-table::
    :header-rows: 1
    :widths: 25 37 38

    * - Question
      - Prefer ``Result``
      - Prefer an exception
    * - Is failure expected?
      - It is an ordinary outcome of the operation.
      - The function cannot fulfil its promised contract.
    * - Where is it handled?
      - The immediate caller can decide what to do.
      - The failure must cross layers to reach a useful boundary.
    * - What information is needed?
      - A small set of actionable status values is enough.
      - Context, a cause chain, or structured diagnostics must be preserved.
    * - How should control flow look?
      - The decision belongs visibly in the local branch.
      - Intermediate callers should not have to forward status values manually.

.. erbsland-demo::
    :source: err/Result/ResultOrException.cpp
    :exec: err/result --demo ResultOrException
    :source-sha256: e5a6fbc466d1f56b82e0d15141305ddcdd12e449261ceb144102eb23f541d128

.. code-block:: cpp

    /// Use `Result` for an expected local outcome that the immediate caller can handle.
    /// Throw an exception when a function cannot produce its promised value and a wider boundary should handle the error.
    void resultOrException() {
        // A blocked safety scanner is an expected state with an immediate response.
        if (enableSafetyScanner(false).isFailure()) {
            el::io::printLine("Move away from the robot before starting."_el);
        }

        // A missing mission prevents this function from returning the promised mission name.
        try {
            el::io::printLine("Mission: "_el, loadMissionOrThrow("Mapear Europa"_el));
        } catch (const el::RuntimeError &error) {
            el::io::printLine("Mission failure: "_el, error.reason());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Move away from the robot before starting.
    Mission failure: The mission plan was not found.

.. erbsland-demo-end::

Where an API offers both forms, Erbsland Core commonly marks the throwing variant with an ``OrThrow`` suffix.
Read :doc:`handling_exceptions` to learn where to throw and catch exceptions, how to preserve their causes, and how an
application turns them into useful reports.
