..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Exceptions; Handling
    single: Exceptions; Cause
    single: Application Error

********************************
Throwing and Handling Exceptions
********************************

This page shows how to use Erbsland exceptions in application code.
You will learn where to catch an error, which text interface to use, how to retain an original failure as a cause, and
how ``Application`` turns an uncaught library exception into a consistent error report.

Catch Errors Where You Can Act
==============================

Throw an exception when a function cannot produce the result promised by its interface.
Catch it where the program can make a meaningful decision: retry an operation, ask for corrected input, select a
fallback, or translate the failure into a higher-level error.

Order handlers from the most specific type to the broadest type.
A local parser can recover from
:cpp:class:`ParseError <erbsland::err::ParseError>` because it understands the input. A subsystem boundary may instead
handle every :cpp:class:`RuntimeError <erbsland::err::RuntimeError>` in the same way.

.. erbsland-demo::
    :source: err/Exception/ThrowAndCatch.cpp
    :exec: err/exception --demo ThrowAndCatch
    :source-sha256: 8af8ca72ce377d4915a322d36a8f06b1b54038011a67926bd84aa302c6cde146

.. code-block:: cpp

    /// A parser throws `ParseError` when its input violates the notation grammar.
    auto parseDynamics(const el::String &text) -> el::String {
        if (text == "piano"_el || text == "forte"_el) {
            return text;
        }
        throw el::ParseError{"The dynamic marking must be 'piano' or 'forte'."_el, el::CpIndex{0}};
    }

    /// Catch the most specific recoverable exception close to the operation that understands it.
    /// Broader runtime-error handlers belong at subsystem or application boundaries, where a generic fallback is useful.
    void throwAndCatch() {
        try {
            el::io::printLine("Dynamics: "_el, parseDynamics("fortissimo"_el));
        } catch (const el::ParseError &error) {
            el::io::printLine("Please correct the notation: "_el, error.toString());
        } catch (const el::RuntimeError &error) {
            el::io::printLine("The score could not be loaded: "_el, error.reason());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Please correct the notation: The dynamic marking must be 'piano' or 'forte'. at code point
     0

.. erbsland-demo-end::

Do not routinely catch :cpp:class:`LogicError <erbsland::err::LogicError>` to continue execution.
It identifies faulty program logic, such as an invalid parameter or an operation outside a supported API range.
Catching it only to suppress the failure can leave the program in a state its author did not design.

Choose the Right Text Interface
===============================

:cpp:class:`Exception <erbsland::err::Exception>` provides several views of its information:

* :cpp:func:`Exception::reason() <erbsland::err::Exception::reason>` returns the reason supplied when the exception was
  created. Use it when adding context or making a programmatic decision that still needs human-readable text.
* :cpp:func:`Exception::toString() <erbsland::err::Exception::toString>` returns a compact description and may add
  exception-specific details, such as a parse position or parameter name.
* :cpp:func:`Exception::what() <erbsland::err::Exception::what>` provides the null-terminated string required by
  ``std::exception``. It intentionally remains a compatibility interface and returns the reason.
* :cpp:func:`Exception::diagnostic() <erbsland::err::Exception::diagnostic>` creates the structured diagnostic for this
  exception. It does not include the exception's cause chain.

.. erbsland-demo::
    :source: err/Exception/ExceptionText.cpp
    :exec: err/exception --demo ExceptionText
    :source-sha256: 439e87ad4359e985cab8706d9e53b8ef93906ea976d2cad45e3e8716c2c43319

.. code-block:: cpp

    /// An exception exposes text for different consumers.
    /// `reason()` is the stable reason supplied by the thrower, `toString()` may add exception-specific details, and
    /// `what()` provides the null-terminated compatibility string expected by standard C++ interfaces.
    void exceptionText() {
        try {
            const auto tempo = el::StringEditor{"速い"_el}.toIntegerOrThrow<int>();
            el::io::printLine("Tempo: "_el, tempo);
        } catch (const el::ParseError &error) {
            el::io::printLine("reason(): "_el, error.reason());
            el::io::printLine("toString(): "_el, error.toString());
            el::io::printLine("what(): "_el, error.what());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    reason(): Expected an integer number, got no digits
    toString(): Expected an integer number, got no digits at code point 0
    what(): Expected an integer number, got no digits

.. erbsland-demo-end::

Prefer structured diagnostics for final user-facing reports.
Concatenating ``toString()`` values loses semantic fields, source locations, rendering styles, and the relationship
between causes.

Preserve the Original Cause
===========================

Lower layers often expose technical details that are not meaningful to the caller.
Translate such a failure into an exception from your own domain, but preserve the active exception with
``std::current_exception()``.
The outer reason then explains which application operation failed, while
:cpp:func:`Exception::cause() <erbsland::err::Exception::cause>` retains the original evidence.

.. erbsland-demo::
    :source: err/Exception/CauseChains.cpp
    :exec: err/exception --demo CauseChains
    :source-sha256: 7252ef01ed1c24f2ba9504356ffae7e8dabbec1fd7c05b8d5fd5bfccb9da5437

.. code-block:: cpp

    /// Preserve a caught failure with `std::current_exception()` when translating it into a domain-level exception.
    /// The outer exception explains the failed operation, while `cause()` retains the original technical failure.
    void causeChains() {
        try {
            try {
                throw std::runtime_error{"audio device disconnected"};
            } catch (...) {
                throw el::RuntimeError{"Shakuhachi recording could not be started."_el, std::current_exception()};
            }
        } catch (const el::RuntimeError &error) {
            el::io::printLine("Outer reason: "_el, error.reason());
            el::io::printLine("Has cause: "_el, error.hasCause());
            try {
                std::rethrow_exception(error.cause());
            } catch (const std::exception &cause) {
                el::io::printLine("Original cause: "_el, cause.what());
            }
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Outer reason: Shakuhachi recording could not be started.
    Has cause: true
    Original cause: audio device disconnected

.. erbsland-demo-end::

The cause is a standard ``std::exception_ptr``.
It may therefore refer to another Erbsland exception, a foreign ``std::exception``, or an unknown exception.
:cpp:class:`DiagnosticHelper <erbsland::err::DiagnosticHelper>` understands all three cases and limits pathological
cause depth while building a complete report.

Let Application Report the Final Error
======================================

:cpp:func:`Application::run() <erbsland::core::Application::run>` is the normal reporting boundary for an Erbsland
application.
It catches :cpp:class:`Exception <erbsland::err::Exception>`, calls ``cleanup()``, builds a diagnostic document
including causes, and renders it through the application's system output.
A callback exception from the automatically managed main event loop stops that loop, shuts down managed event threads,
and then reaches this same boundary.
A foreign ``std::exception`` is not caught directly by ``run()``, but it is rendered when preserved as the cause of an
Erbsland exception.

Use :cpp:class:`ApplicationError <erbsland::core::ApplicationError>` when application code wants to choose the final
exit code or provide a title, description, source, and code location.
Build those optional details in an
:cpp:class:`ApplicationErrorContext <erbsland::core::ApplicationErrorContext>`.

.. erbsland-demo::
    :source: err/Exception/ApplicationReporting.cpp
    :exec: err/exception --demo ApplicationReporting
    :exec-exit-code: 1
    :source-sha256: 03a061f5d00a2db7eaf0c072d14669fd7b70f06edef8a04c6d7e16d0b0b528f4

.. code-block:: cpp

    /// Throw `ApplicationError` from code executed by `Application::run()` to request consistent error reporting.
    /// Its context provides a title, description, source information, location, and the process exit code.
    void applicationReporting() {
        auto context = el::ApplicationErrorContext{
            "The score could not be read."_el,
            "The main melody contains an unrecognized symbol."_el,
            el::ExitCode::failure(),
        };
        context.setSourceName("春の合奏"_el)
            .setSourcePath("scores/春の合奏.music"_el)
            .setCodeLocation(el::CodeLocation{el::LineIndex{11}, el::ColumnIndex{8}});
        throw el::ApplicationError{std::move(context), {}};
    }

.. erbsland-ansi::
    :escape-char: ␛

    Error: The score could not be read.
      The main melody contains an unrecognized symbol.
    Error Source:
      Source: 春の合奏
      Path:   scores/春の合奏.music
      Line:   12
      Column: 9

.. erbsland-demo-end::

Use ``ApplicationError`` for a failure that should end the current application run with an intentional report.
For a recoverable library or domain operation, keep the more specific runtime exception and let the caller decide
whether it can continue.

The next page, :doc:`diagnostics`, explains how to build the same reports manually when you are not using
``Application::run()`` or need a different reporting boundary.
