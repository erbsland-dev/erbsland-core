***********************************
Configuration Domain API Guidelines
***********************************

Core Semantics
==============

Document Model
--------------

.. code-block:: text

    document = immutable rooted tree of typed configuration values
    section = named map, ordered list, or text-indexed map of child values
    scalar = integer, boolean, float, text, bytes, calendar value, or regular expression
    name path = absolute or relative sequence of configuration names
    location = source identifier plus line and column of a parsed value

Source Model
------------

.. code-block:: text

    source = configuration input with a stable identity
    resolution = deterministic expansion of an include relative to its containing source
    source authorization = independent trust decision for every resolved source
    signature validation = application trust decision over the parser's exact digest and signature data

Primary Types
=============

.. code-block:: text

    Value // immutable node in a parsed configuration tree
    Document // root value and metadata of one configuration document
    Parser // ELCL parser and policy coordinator

Value and Document Types
========================

.. code-block:: text

    Integer, Float // configuration scalar numeric representations
    Name, NamePath // configuration key component and complete value path
    Location // source location attached to a parsed value
    DocumentBuilder // programmatic configuration document builder
    ValueIterator, ValueList, ValueMatrix // tree traversal and aggregate value types
    Matrix❮Value❯ // rectangular aggregate with per-row defined lengths
    ValueType, NameType // value and name classifications

Source and Trust Types
======================

.. code-block:: text

    Source, SourceResolver, FileSourceResolver // configuration input and include resolution
    SourceIdentifier, SourceResolverContext // stable source identity and include context
    AccessCheck, FileAccessCheck // custom and file-based source authorization
    AccessCheckResult, AccessSources // access decision and source classification
    Signer, SignatureSigner // document signing service and application signature callback
    SignatureValidator // application signature validation callback
    SignatureSignerData, SignatureValidatorData // exact signing and validation inputs
    SignatureValidatorResult // accepted, rejected, or unsupported signature result

Error Types
===========

.. code-block:: text

    ConfError // configuration parsing, access, validation, or signing failure
    ConfErrorContext // structured configuration diagnostic context
    ConfErrorCategory // stable configuration failure category

Pattern Definitions
===================

.. code-block:: text

    C = ❮CoreType❯ // matching Core scalar type
    Vp = ValuePtr/ValueConstPtr // shared mutable or immutable configuration value

Value Access Patterns
=====================

.. code-block:: text

    o.type()/name()/namePath()/location() -> T // inspect a value's identity and source metadata
    o.as❮Type❯() -> C // get a scalar value or its documented fallback
    o.as❮Type❯OrThrow() -> C // get a scalar value or throw ConfError for a type mismatch
    o.value(path) -> Vp // find a child value or return an empty pointer
    o.valueOrThrow(path) -> Vp // find a child value or throw ConfError
    o.get❮Type❯(path[, default]) -> C // get a typed child or a fallback
    o.get❮Type❯OrThrow(path) -> C // get a typed child or throw ConfError
    o.begin()/end() -> ValueIterator // iterate direct children without flattening the tree

Document Construction Patterns
==============================

.. code-block:: text

    o.addSectionMap/addSectionList(path) // add a section container
    o.add❮Type❯(path, value) // add a scalar using the shared scalar mapping
    o.document() -> DocumentPtr // finish or access the built document
    o.reset() // discard the current construction state
    o.toFlatValueMap() -> ValueMap // index all values by absolute name path

Parsing Patterns
================

.. code-block:: text

    o.parse(source) -> DocumentPtr // parse or return an empty pointer and retain the error context
    o.parseOrThrow(source) -> DocumentPtr // parse or throw ConfError
    o.parseFile/parseText(input) -> DocumentPtr // convenience parsing with non-throwing failure reporting
    o.parseFileOrThrow/parseTextOrThrow(input) -> DocumentPtr // convenience parsing with exceptions
    o.lastError() -> ConfErrorContext // inspect the latest non-throwing parse failure
    o.setSourceResolver/setAccessCheck/setSignatureValidator(policy) // install parsing trust policies

Source and Trust Patterns
=========================

.. code-block:: text

    T::fromString/fromFile(input) -> SourcePtr // create a closed in-memory or file source
    o.resolve(context) -> SourceList // resolve an include into deterministic candidate sources
    o.check(source, context) -> AccessCheckResult // authorize one source independently
    o.validate(data) -> SignatureValidatorResult // validate exact parser-produced signature data
    o.sign(source, destination, signer) // insert or replace a document signature
