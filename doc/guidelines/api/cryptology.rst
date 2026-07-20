********************************
Cryptology Domain API Guidelines
********************************

These guidelines extend the Common, Memory, Text, and Unit API Guidelines for cryptographic algorithms and stateful
operations.

The purpose of this document is to define a base naming vocabulary for cryptology APIs.
It is intentionally plain, technical, and list-based to provide a quick overview of method names and their usage
patterns.
If new vocabulary is introduced, update this page to provide a reference for future extensions.

Core Semantics
==============

Algorithm Policy
----------------

.. code-block:: text

    CryptographicStatus // current library policy: Disallowed, Legacy, or Acceptable
    CryptographicSecurity // coarse selection level: Standard or High
    HashThroughput // hash-specific relative throughput: Low, Medium, or High
    stable identifier // persisted canonical algorithm name; does not change with recommendation metadata
    recommendation metadata // versioned library policy that can change as cryptographic guidance evolves

Hash Lifecycle
--------------

.. code-block:: text

    invalid Hasher // default placeholder; operations except isValid() throw err::LogicError
    active Hasher // accepts zero or more update() calls
    finalized Hasher // finalize() returns cached digest; update() requires reset() first
    copied Hasher // shares state and detaches before the next mutation

Primary Types
=============

.. code-block:: text

    HashAlgorithm // fixed-output cryptographic hash algorithm and selection metadata
    Hasher // copy-on-write streaming hash state
    HashRequirements // explicit status, security, and throughput selection requirements

Selection Types
===============

.. code-block:: text

    CryptographicStatus // whether an algorithm is allowed for new results
    CryptographicSecurity // coarse security level for user-facing selection
    HashThroughput // relative throughput among fixed-output streaming hashes

Algorithm Patterns
==================

.. code-block:: text

    o.digestSize() -> unit::ByteLength // fixed digest size
    o.status() -> CryptographicStatus // current usability policy
    o.security() -> CryptographicSecurity // coarse security level
    o.throughput() -> HashThroughput // current relative implementation throughput
    o.isSafe() -> bool // acceptable with at least standard security
    o.matches(requirements) -> bool // test all explicit selection requirements
    HashAlgorithm::all/allSafe() -> span // enumerate stable algorithm lists
    HashAlgorithm::matching(requirements) -> util::List<HashAlgorithm> // enumerate requirement matches
    HashAlgorithm::recommended(requirements) -> optional<HashAlgorithm> // select the preferred matching algorithm
    o.toString() -> text::String // stable lowercase algorithm identifier
    HashAlgorithm::fromString(text) -> optional<HashAlgorithm> // parse an exact canonical identifier
    HashAlgorithm::fromStringOrThrow(text) -> HashAlgorithm // parse or throw err::ParseError

Hasher Patterns
===============

.. code-block:: text

    Hasher() // create an invalid placeholder
    Hasher(algorithm) // create active state for an algorithm
    o.isValid() -> bool // test whether an algorithm and worker are present
    o.algorithm() -> HashAlgorithm // configured algorithm; invalid state throws
    o.reset() // begin a new stream with the same algorithm
    o.update(byte span/ByteBlock) // add exact binary bytes
    o.update(text::String) // add exact stored UTF-8 bytes without conversion
    o.finalize() -> mem::ByteBlock // finalize once or return the cached digest
