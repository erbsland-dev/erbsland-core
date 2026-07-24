********************************
Cryptology Domain API Guidelines
********************************

Core Semantics
==============

Algorithm Policy
----------------

.. code-block:: text

    algorithm status = disallowed, legacy verification only, or acceptable for new results
    security level = standard or high user-facing selection threshold
    throughput = low, medium, or high relative hashing speed
    stable identifier = persisted canonical name independent of changing recommendation metadata
    recommendation = versioned library policy ordered by security family and implementation properties

Hash Lifecycle
--------------

.. code-block:: text

    invalid state = algorithm-less placeholder that accepts no hashing operations
    active state = zero or more message updates before finalization
    finalized state = immutable cached digest until reset
    copied state = shared state that detaches before mutation

Password Hashing
----------------

.. code-block:: text

    password record = immutable canonical database or configuration boundary
    password input = UTF-8 text whose allocation should be marked as sensitive
    application key = at least 32 marked bytes, optionally identified for rotation
    reviewed policy = supported algorithm and cost preset for new records
    replacement record = successful verification result upgraded for format, policy, mode, or key changes
    unsafe policy = explicit custom-cost or unkeyed escape hatch isolated from ordinary API lookup

Primary Types
=============

.. code-block:: text

    HashAlgorithm // fixed-output cryptographic hash algorithm
    Hasher // copy-on-write streaming hash state
    PasswordHash // opaque canonical password storage record
    PasswordHasher // password record creation and verification service

Secondary Types
===============

.. code-block:: text

    HashRequirements // explicit status, security, and throughput selection requirements
    CryptographicStatus, CryptographicSecurity, HashThroughput // algorithm selection metadata
    PasswordHashAlgorithm // supported password-hashing algorithm identifier
    PasswordHashKey // marked application pepper and optional public rotation identifier
    PasswordHashPolicy // reviewed password-hashing construction and cost preset
    PasswordVerification // explicit verification and migration result

Hash Selection Patterns
=======================

.. code-block:: text

    o.digestSize() -> unit::ByteLength // get the fixed digest size
    o.status()/security()/throughput() -> T // inspect current selection metadata
    o.matches(requirements) -> bool // test every explicit selection requirement
    T::all/allSafe() -> List<HashAlgorithm> // enumerate stable algorithm sets
    T::matching/recommended(requirements) -> T // list or select requirement matches
    o.toString() -> text::String // get the stable lowercase algorithm identifier
    T::fromString/fromStringOrThrow(text) -> T // parse an exact canonical identifier

Hashing Patterns
================

.. code-block:: text

    T(algorithm) // create active state for an algorithm
    o.isValid() -> bool // test whether hashing state is present
    o.reset()/secureErase() // begin a new stream, optionally erasing the previous message state
    o.update(bytes/text) // append exact bytes without representation conversion
    o.finalize() -> mem::ByteBlock // finalize once or return the cached digest

Password Hashing Patterns
=========================

.. code-block:: text

    T(key[, policy]) // create a password hasher using reviewed defaults
    T::withKeyRotation(active, fallbacks[, policy]) -> T // configure an active key and bounded legacy keys
    o.hash(text::String) -> PasswordHash // create a salted canonical record from preferably marked text
    o.verify(password, record) -> PasswordVerification // return acceptance and any migration record
    T::fromString/fromStringOrThrow(text) -> PasswordHash // parse a canonical storage record
    o.toString() -> text::String // create the canonical storage representation
    o.replacementHash() -> PasswordHash // access an upgraded record after accepted verification
