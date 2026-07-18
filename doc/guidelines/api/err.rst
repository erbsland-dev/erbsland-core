***************************
Error Domain API Guidelines
***************************

These guidelines extend the Common API Guidelines for public APIs that report errors with typed exceptions, structured
diagnostics and diagnostic cause chains.

The ``err`` namespace is the neutral error foundation of the library.
It owns the common exception base classes, cross-domain error categories and the abstract diagnostic interface.
Domain-specific exceptions live in their owning domain namespace, so concrete diagnostics can refer to domain types
without creating circular dependencies.

Core Semantics
==============

Exception Model
---------------

.. code-block:: text

    Exception // common base for all library exceptions
    reason // human-readable error text stored as text::String
    cause // optional std::exception_ptr with extended diagnostic cause
    what() // std::exception compatible null-terminated reason text
    toString() // local plain text for one exception, without rendering causes
    diagnostic() // abstract diagnostic for one exception
    DiagnosticConstPtr // immutable shared diagnostic pointer
    TextDocument // neutral output tree for rendered diagnostics
    DomainError // typed exception owned by the domain that reports the failure
    OperationOrThrow() // explicit throwing variant of an otherwise non-throwing API

Diagnostic Trust Boundary
-------------------------

*   Developer-authored titles, descriptions, help text, option contexts and display-map values are trusted
    and rendered verbatim.
*   Library-owned builders must escape values originating outside the application, including path text, command-line
    arguments, native messages, and foreign exception text.
*   ``EscapeFormat::Display`` is used for readable output that cannot inject terminal control sequences.

Usage of Cause Chain
--------------------

*   ``cause`` is only used for a genuinely separate failure layer.
*   Most prominent example: :cpp:class:`ApplicationError <erbsland::core::ApplicationError>` transporting the actual cause.
*   In library errors should fold and explain platform errors to keep exceptions chains small and
    diagnostics useful for the user. (E.g. by embedding :cpp:class:`PlatformErrorContext <erbsland::system::PlatformErrorContext>`)

Writing Style of Error Messages
-------------------------------

Domain reasons stay clean and user-facing, answering the most important questions of the user:

*   **What** went wrong? (title)
*   **Why** did it fail? (description)
*   Optional: **How** can the problem get resolved? (help)

Neutral Error Types
===================

.. code-block:: text

    Exception // base class for all Erbsland exceptions
    LogicError // violated programming precondition or invariant
    RuntimeError // domain-neutral runtime failure
    ParameterError // invalid API argument
    OutOfRangeError // requested value, index or range is outside accepted bounds
    OverflowError // arithmetic or conversion result cannot be represented
    ParseError // text cannot be parsed as the requested value

Diagnostic Types
================

.. code-block:: text

    Diagnostic // abstract interface for one error
    ErrorDocumentBuilder // shared builder for consistently styled error documents

Domain Errors
=============

.. code-block:: text

    ❮domain❯::❮Type❯Error // domain specific error

Exception and Domain Exception Patterns
=======================================

.. code-block:: text

    Exception() // create an exception with empty reason
    Exception(reason[, cause]) // create an exception with reason and optional cause
    ❮Type❯Error(reason[, cause]) // Error with reason and cause
    ❮Type❯Error(context[, cause]) // Using a context to store more info than reason alone.
    o.what() -> mem::UnsafeConstCharPtr // std::exception reason text
    o.reason() -> const text::String& // stored human-readable reason
    o.hasCause() -> bool // test if a chained diagnostic cause is available
    o.cause() -> std::exception_ptr // access the chained diagnostic cause
    o.toString() -> text::String // local display text for one exception
    o.diagnostic() -> DiagnosticConstPtr // abstract diagnostic for one exception
    o.toTextDocument([displayText]) -> TextDocument // render one diagnostic with optional display text

Context to Provide Additional Error Data
========================================

.. code-block:: text

    ❮Type❯ErrorContext(title [, description]) // common ctor for context
    ❮Type❯ErrorContext(title, domainValue) // domain specific data
    o.setDescription(text) -> T& // chained setters to build context
    o.set❮DomainData❯(data) -> T& // adding domain specific data
    o.❮domainData❯() -> V // accessing the data for building diagnostics

Throw Helper Patterns
=====================

*   Throw helpers are implementation details for domains that need to break include cycles.
*   Throw helpers are **implementation detail**, not public API.
*   Prefer direct ``throw`` in ordinary code.

.. code-block:: text

    ❮domain❯::impl::throw❮Error❯(...) // placed in `❮domain❯/impl/ThrowHelper.hpp`
