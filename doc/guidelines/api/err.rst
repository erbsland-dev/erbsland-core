***************************
Error Domain API Guidelines
***************************

Core Semantics
==============

Exception Model
---------------

.. code-block:: text

    exception = throwable failure with a human-readable local reason
    diagnostic = structured, renderable representation of one failure
    domain error = typed exception owned by the failing domain
    reason = human-readable local error text
    cause = optional exception from a genuinely separate failure layer

Diagnostic Trust Boundary
-------------------------

.. code-block:: text

    developer-authored text = trusted and rendered verbatim
    external values = display-escaped before insertion into diagnostics
    native or foreign text = untrusted external value

Usage of Cause Chain
--------------------

.. code-block:: text

    cause = failure from a separate abstraction layer
    platform context = part of its owning domain diagnostic, not a routine cause

Writing Style of Error Messages
-------------------------------

.. code-block:: text

    title = what failed
    description = why it failed
    help = optional resolution
    external values = display-escaped before insertion

Primary Types
=============

.. code-block:: text

    Exception // base class for all Erbsland exceptions
    Diagnostic // abstract structured representation of one failure

Secondary Types
===============

.. code-block:: text

    LogicError, RuntimeError // programming and domain-neutral runtime failures
    ParameterError, OutOfRangeError // invalid argument and bounds failures
    OverflowError, ParseError // representation and text-parsing failures
    ErrorDocumentBuilder // shared builder for consistently styled error documents
    DiagnosticHelper // adapter from exceptions and cause chains to diagnostic documents
    ❮domain❯::❮Type❯Error // domain specific error

Pattern Definitions
===================

.. code-block:: text

    V = ❮DiagnosticData❯ // domain-specific diagnostic data

Exception Patterns
==================

.. code-block:: text

    T() // create an exception with an empty reason
    T(reason[, cause]) // create an exception with a reason and optional cause
    T(context[, cause]) // create a domain exception from structured context
    o.what() -> mem::UnsafeConstCharPtr // expose the reason at the standard exception boundary
    o.reason() -> const text::String& // stored human-readable reason
    o.hasCause() -> bool // test if a chained diagnostic cause is available
    o.cause() -> std::exception_ptr // access the chained diagnostic cause
    o.toString() -> text::String // local display text for one exception
    o.diagnostic() -> DiagnosticConstPtr // abstract diagnostic for one exception
    o.toTextDocument([displayText]) -> text::TextDocument // render one diagnostic with optional display text

Error Context Patterns
======================

.. code-block:: text

    T(title[, description]) // create context with common diagnostic text
    T(title, domainValue) // create context with domain-specific data
    o.setDescription(text) -> T& // chained setters to build context
    o.set❮DomainData❯(data) -> T& // adding domain specific data
    o.❮domainData❯() -> V // accessing the data for building diagnostics

Throw Helper Patterns
=====================

.. code-block:: text

    throw❮Error❯(...) // implementation helper used only to break include cycles
    ❮operation❯OrThrow(...) -> T // throwing variant of an otherwise non-throwing operation
