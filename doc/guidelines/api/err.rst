***************************
Error Domain API Guidelines
***************************

Core Semantics
==============

.. code-block:: text

    exception = throwable failure with a human-readable local reason
    diagnostic = structured, renderable representation of one failure
    error = recoverable runtime error
    failure = fatal failure with no practical recovery
    cause = optional exception from a genuinely separate failure layer
    context = additional, optional error details owned by the exception for detailed diagnostics

Primary Types
=============

.. code-block:: text

    Exception // base class for all Erbsland Core exceptions
    Diagnostic // abstract structured representation of one failure

Secondary Types
===============

.. code-block:: text

    LogicFailure, RuntimeError // programming and domain-neutral runtime failures
    ParameterFailure, OutOfRangeFailure // caller precondition and system-capacity failures
    OutOfRangeError, NotFoundError // recoverable numeric bounds and absent key/name errors
    OverflowError, ParseError // representation and text-parsing failures
    ErrorDocumentBuilder // shared builder for consistently styled error documents
    DiagnosticHelper // adapter from exceptions and cause chains to diagnostic documents
    ❮domain❯::❮Type❯Error // recoverable domain error, derived from RuntimeError
    ❮domain❯::❮Type❯Failure // fatal domain failure, derived from LogicFailure

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
    o.toTextDocument() -> text::TextDocument // render one diagnostic using application display texts

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
