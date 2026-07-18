..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Error Handling; Overview
    single: Exceptions; Overview
    single: Exceptions; OrThrow
    single: Result
    single: Exit Code

***********************
Error Handling Overview
***********************

This page introduces the failure model used throughout Erbsland Core.
You will learn when to return a status, when to use an exit code, how the common exception hierarchy is organized, and
why individual domains provide more specific error types.

Choosing How to Report Failure
==============================

Not every unsuccessful operation is exceptional.
Choose the smallest mechanism that preserves the information the caller needs:

* :cpp:class:`Result <erbsland::util::Result>` is a compact, type-safe status value. It works well when success and
  failure are ordinary outcomes and the immediate caller decides what to do next.
* :cpp:class:`ExitCode <erbsland::unit::ExitCode>` represents the final status of an application or tool. It belongs at
  a process boundary rather than deep inside application logic.
* :cpp:class:`Exception <erbsland::err::Exception>` reports that a function could not fulfil its contract. Exceptions
  can cross several call layers, retain an original cause, and produce structured diagnostics.

.. erbsland-demo::
    :source: err/Exception/ChoosingFailureMechanism.cpp
    :exec: err/exception --demo ChoosingFailureMechanism
    :exec-exit-code: 3
    :source-sha256: 4425147ba48a9b5638cef638bdd956df346145228f8614ce2da12d558edf8247

.. code-block:: cpp

    /// Erbsland Core offers three complementary ways to report failure.
    /// Use `Result` for a small status returned to the immediate caller, `ExitCode` at a process boundary, and an
    /// exception when a function cannot produce its promised value and callers may add diagnostic context.
    void choosingFailureMechanism() {
        // A result makes an expected local outcome part of normal control flow.
        if (isSuccessful(prepareInstrument("尺八"_el))) {
            el::io::printLine("The shakuhachi is ready."_el);
        }

        // The throwing variant lets a more distant caller handle the failure.
        try {
            prepareInstrumentOrThrow("篳篥"_el);
        } catch (const el::RuntimeError &error) {
            el::io::printLine("Exception: "_el, error.reason());
        }

        // ApplicationError transports the final process exit code to Application::run().
        throw el::ApplicationError{"The concert preparation could not be completed."_el, el::ExitCode{3}};
    }

    auto prepareInstrument(const el::String &instrument) noexcept -> el::Result {
        return instrument == "尺八"_el ? el::Result::Success : el::Result::Failure;
    }

    void prepareInstrumentOrThrow(const el::String &instrument) {
        if (isFailure(prepareInstrument(instrument))) {
            throw el::RuntimeError{el::String::fromJoined({"The instrument could not be prepared: "_el, instrument})};
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    The shakuhachi is ready.
    Exception: The instrument could not be prepared: 篳篥
    Error: The concert preparation could not be completed.

.. erbsland-demo-end::

This choice is about control flow, not severity.
A failed lookup may be an expected ``Result`` in one API and an exception in another API that promises to return a
value.
Keep the contract obvious to the caller and avoid encoding failure in magic values.
Read :doc:`reporting_with_result` for practical handling patterns and for guidance on writing custom result types.

Why Throwing Variants End in ``OrThrow``
----------------------------------------

Where practical, Erbsland Core makes regular methods non-throwing.
If an operation also has a variant that reports failure through an exception, the throwing name ends in
``...OrThrow()``.
The suffix makes exceptional control flow visible at the call site and lets you choose the variant that matches the
current layer.

For example, :cpp:func:`String::toInteger() <erbsland::text::U8String::toInteger>` returns a caller-supplied fallback
when conversion fails, while
:cpp:func:`String::toIntegerOrThrow() <erbsland::text::U8String::toIntegerOrThrow>` reports the parse failure.
Likewise, :cpp:func:`PathContent::readText() <erbsland::path::PathContent::readText>` returns an empty optional on
error, while :cpp:func:`PathContent::readTextOrThrow() <erbsland::path::PathContent::readTextOrThrow>` preserves the
failure as an exception and diagnostic.

The suffix marks a throwing alternative to an otherwise non-throwing operation; it is not required on every function
that can throw.
Some classes are documented as generally throwing because failures can occur throughout their API—for example, when a
parser reads from an embedded input object.
For those APIs, the class-level contract defines the expected exceptions unless an individual method is ``noexcept``.

The Common Exception Family
===========================

Every library exception derives from :cpp:class:`Exception <erbsland::err::Exception>`.
The two main branches express whether application code is expected to recover:

* :cpp:class:`LogicError <erbsland::err::LogicError>` indicates a broken program invariant or invalid API use. These
  errors should normally reach the application boundary and terminate the faulty operation instead of being treated as
  user input problems.
* :cpp:class:`RuntimeError <erbsland::err::RuntimeError>` represents failures caused by input, data, resources, or the
  execution environment. Catch these errors where your application can recover or add useful context.

.. mermaid::

    classDiagram
        Exception <|-- LogicError
        Exception <|-- RuntimeError
        LogicError <|-- OutOfRangeError
        LogicError <|-- ParameterError
        RuntimeError <|-- OverflowError
        RuntimeError <|-- ParseError

The neutral derived types cover recurring situations without introducing a domain dependency:

* :cpp:class:`ParameterError <erbsland::err::ParameterError>` identifies an invalid function argument and includes its
  parameter name.
* :cpp:class:`OutOfRangeError <erbsland::err::OutOfRangeError>` reports an operation outside a valid API range.
* :cpp:class:`OverflowError <erbsland::err::OverflowError>` reports a runtime calculation that exceeded its supported
  range.
* :cpp:class:`ParseError <erbsland::err::ParseError>` reports invalid text and can identify a code-point position.

.. erbsland-demo::
    :source: err/Exception/BuiltInExceptions.cpp
    :exec: err/exception --demo BuiltInExceptions
    :source-sha256: 26e4983b7beeabc14fd3045033266a7ced64a7f564b95b42dac607d2c012f20a

.. code-block:: cpp

    /// The neutral exception types describe common failure categories without tying them to a library domain.
    /// `LogicError` and `ParameterError` indicate programming mistakes, while `RuntimeError`, `OverflowError`, and
    /// `ParseError` describe failures that can arise from data or the execution environment.
    void builtInExceptions() {
        const auto parameterError = el::ParameterError{"The volume must be positive."_el, "volume"_el};
        const auto parseError = el::ParseError{"The musical note could not be parsed."_el, el::CpIndex{4}};
        const auto overflowError = el::OverflowError{"The time-signature counter exceeded its limit."_el};

        el::io::printLine("Parameter: "_el, parameterError.toString());
        el::io::printLine("Parse: "_el, parseError.toString());
        el::io::printLine("Overflow: "_el, overflowError.toString());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Parameter: The volume must be positive. (parameter: volume)
    Parse: The musical note could not be parsed. at code point 4
    Overflow: The time-signature counter exceeded its limit.

.. erbsland-demo-end::

The hierarchy is deliberately independent from similarly named standard-library exceptions.
Catch Erbsland exceptions through their Erbsland base classes, and use the actual inheritance shown above when selecting
a handler.

Domain-Specific Exceptions
==========================

A neutral exception tells you the broad kind of failure.
A domain-specific exception additionally identifies the operation and can carry structured context meaningful to that
domain.
Examples include:

* :cpp:class:`FormatError <erbsland::text::FormatError>` and
  :cpp:class:`EncodingError <erbsland::text::EncodingError>` from the text domain,
* :cpp:class:`PathError <erbsland::path::PathError>` from the path domain,
* :cpp:class:`OptionError <erbsland::options::OptionError>` from command-line processing,
* :cpp:class:`PlatformError <erbsland::system::PlatformError>` for native operating-system failures, and
* :cpp:class:`StreamError <erbsland::stream::StreamError>` for input and output operations.

Catch the domain type when you can respond to that particular failure.
Catch its neutral base at a wider boundary when all runtime failures have the same fallback.

.. erbsland-demo::
    :source: err/Exception/DomainExceptions.cpp
    :exec: err/exception --demo DomainExceptions
    :source-sha256: 8b0650fa6a78f82bbfc6de84e14f1d293f2ce8442134794c0dc4057eb5f0e13f

.. code-block:: cpp

    /// Domain-specific exceptions add meaning and diagnostic data while preserving the common `RuntimeError` contract.
    /// Catch the most specific type when you can recover from that domain failure, or `RuntimeError` at a wider boundary.
    void domainExceptions() {
        // An integer field rejects a text argument and raises the text-domain exception.
        try {
            const auto description = el::StringFormat{"Tempo: {:d}"_el}.build("速い"_el);
            el::io::printLine(description);
        } catch (const el::FormatError &error) {
            el::io::printLine("Text-domain error: "_el, error.reason());
        }

        // A child that was never created raises the path-domain exception when read.
        try {
            const auto missingScore = el::Path::currentDirectory().joined("__erbsland_missing_存在しない楽譜__.music"_el);
            const auto score = missingScore.content().readTextOrThrow();
            el::io::printLine(score);
        } catch (const el::PathError &error) {
            el::io::printLine("Path-domain error: "_el, error.reason());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Text-domain error: Format field requires an integer argument
    Path-domain error: File could not be opened for reading

.. erbsland-demo-end::

The domain exception remains the right type when it contains details such as a path, option, or native error code.
Do not flatten those details into a string merely to throw a neutral ``RuntimeError``.

From Failure to a Helpful Message
=================================

An exception carries control flow and error context.
A :cpp:class:`Diagnostic <erbsland::err::Diagnostic>` translates that information into a user-facing structure, and a
:cpp:class:`TextDocument <erbsland::text::TextDocument>` keeps the result independent from its final renderer.
This separation lets the same failure become plain log text or a styled terminal message.

Continue with :doc:`reporting_with_result` for expected local outcomes, or :doc:`handling_exceptions` for practical
throwing, catching, and cause chaining.
Read :doc:`diagnostics` when you need to inspect or render diagnostic data, and :doc:`writing_custom_exceptions` when
your own domain needs a dedicated error type.
