***************************
Error Domain API Guidelines
***************************

These guidelines extend the Common API Guidelines for public APIs that report errors with typed exceptions and
throw-helper functions.

The purpose of this document is to define a base naming vocabulary for error APIs.
It is intentionally plain, technical and list based to get a quick overview of method names and their usage patterns.
If you introduce new vocabulary, update this page to provide a good reference for future extensions.

Core Semantics
==============

Exception Model
---------------

.. code-block:: text

    Exception // common base for all library exceptions
    reason // human-readable error text stored as text::StringView
    what() // std::exception compatible null-terminated reason text
    toString() // full error text including type-specific details
    ❮Domain❯Error // typed exception for a failing subsystem or error category
    ❮Operation❯OrThrow() // explicit throwing variant of an otherwise non-throwing API

Primary Types
=============

.. code-block:: text

    Exception // base class for all Erbsland exceptions
    ApplicationError // application termination error with exit code
    OptionError // command-line option processing error with context
    ParseError // text cannot be parsed as the requested value
    ParseNumberError // numeric parse error with machine-readable reader status
    FormatError // format pattern, argument, or safety-limit error
    EncodingError // base class for requested exception-based encoding failures
    OutOfRangeError // requested value, index, or range is outside accepted bounds
    OverflowError // arithmetic or conversion result cannot be represented
    RandomError // random source cannot provide requested data
    StreamError // stream operation cannot complete

Secondary Types
===============

.. code-block:: text

    U8EncodingError // UTF-8 encoding error with byte index
    U16EncodingError // UTF-16 encoding error with data-unit index
    U32EncodingError // UTF-32 encoding error with code-point index

Construction Patterns
=====================

.. code-block:: text

    Exception() // create an exception with empty reason
    Exception(reason) // create an exception with human-readable reason text
    ❮Domain❯Error(reason) // create a typed exception with reason text
    ParseError(reason[, position]) // create parse error with optional code-point position
    ParseNumberError(reason, status[, position]) // create number parse error with reader status
    ApplicationError(reason[, exitCode]) // create application error with termination exit code
    OptionError(context) // create option error from structured option context
    U❮width❯EncodingError(reason, index) // create encoding error pinned to a data index

Exception Access Patterns
=========================

.. code-block:: text

    o.what() -> mem::UnsafeConstCharPtr // std::exception reason text
    o.reason() -> const text::StringView& // stored human-readable reason
    o.toString() -> text::StringView // complete display text for an error
    o.position() -> unit::CpIndex // optional parse error position
    o.status() -> text::ReadNumberStatus // number reader status for ParseNumberError
    o.exitCode() -> unit::ExitCode // application termination exit code
    o.context() -> options::OptionErrorContext // structured option error context

Throw Helper Patterns
=====================

.. code-block:: text

    throw❮Domain❯Error(reason) // throw a typed exception through a dependency-light helper
    throwEncodingError(reason) // throw err::EncodingError
    throwU❮width❯EncodingError(reason, index) // throw a UTF encoding error with data index
    throwFormatError(reason) // throw err::FormatError
    throwOutOfRange(reason) // throw err::OutOfRangeError
    throwOverflow(reason) // throw err::OverflowError
    throwParseError(reason) // throw err::ParseError
    throwParseNumberError(reason, status[, position]) // throw err::ParseNumberError

Failure Category Patterns
=========================

.. code-block:: text

    err::ParseError // malformed text or text that cannot represent the requested value
    err::ParseNumberError // numeric parse failure when reader status is useful
    err::OutOfRangeError // syntactically valid value outside accepted bounds
    err::OverflowError // arithmetic or conversion overflow
    err::EncodingError // invalid text data when exception mode is requested
    err::FormatError // invalid format pattern, argument mismatch, or formatting safety limit
    err::RandomError // operating-system entropy or random source failure
    err::StreamError // read, write, flush, seek, or close failure
    err::OptionError // command-line definition, parsing, validation, or callback failure
