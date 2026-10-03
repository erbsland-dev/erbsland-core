***********************************
Configuration Domain API Guidelines
***********************************

Core Semantics
==============

.. code-block:: text

    document = immutable rooted tree of typed configuration values
    section = named map, ordered list, or text-indexed map of child values
    name path = absolute or relative sequence of configuration names
    source = configuration input with a stable identity
    source authorization = independent trust decision for every resolved source
    syntax snapshots = lossless source in explicitly included conf::syntax, without namespace flattening
    signature validation = application trust decision over the parser's exact digest and signature data

Validation Rule Model
---------------------

.. code-block:: text

    root rule = implicit rule for the complete document or section branch passed to validation

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

Validation Rules Types
======================

.. code-block:: text

    Rules // finalized, reusable validation schema
    RulesBuilder // public programmatic construction entry point
    RulesBuilderBranch // relative path view sharing a builder backend
    RuleDefinition // stable mutation interface supplied to builder attributes
    Rule, Constraint // read-only validated rule metadata
    RuleType, ConstraintType, DependencyMode // rule, constraint, and dependency classifications

Error Types
===========

.. code-block:: text

    ConfError // configuration parsing, access, validation, or signing failure
    ConfErrorContext // structured configuration diagnostic context
    ConfErrorCategory // stable configuration failure category

Syntax Formatting Types
=======================

.. code-block:: text

    syntax::Document // immutable original source, tokens, statements, lines and diagnostics
    syntax::Parser // recovering source parser without semantic evaluation
    syntax::ValueWrapping // placement policy for complete non-multiline values
    syntax::Formatter // source-preserving layout engine
    syntax::Format // base style and selector rules
    syntax::FormatStyle, syntax::FormatStylePatch // complete properties, syntax::explicit overrides
    syntax::Selector, syntax::FormatRule // conjunctive conditions, syntax::selector and property patch
    syntax::Token, syntax::TokenKind // exact source slices and lexical classifications
    syntax::DocumentPtr, syntax::TokenPtr // shared ownership of read-only syntax snapshots and tokens
    syntax::Statement, syntax::Diagnostic, syntax::FormatResult // source structure, errors and formatted output

Pattern Definitions
===================

.. code-block:: text

    C = ❮CoreType❯ // matching Core scalar type
    Vp = ValuePtr/ValueConstPtr // shared mutable or immutable configuration value

Value Access Patterns
=====================

.. code-block:: text

    o.type()/name()/namePath()/location() -> T // inspect a value's identity and source metadata
    o.digest(algorithm) -> ByteBlock // local digest of names, types, contents and configured order
    o.as❮Type❯() -> C // get a scalar value or its documented fallback
    o.as❮Type❯OrThrow() -> C // get a scalar value or throw ConfError for a type mismatch
    o.value(path) -> Vp // find a child value or return an empty pointer
    o.valueOrThrow(path) -> Vp // find a child value or throw ConfError
    o.get❮Type❯(path[, default]) -> C // get a typed child or a fallback
    o.get❮Type❯OrThrow(path) -> C // get a typed child or throw ConfError
    o.asList❮C❯()/asListOrThrow❮C❯() -> List❮C❯ // convert a uniform list or matching scalar
    o.getList❮C❯(path[, default]) -> List❮C❯ // read a uniform list or return the supplied fallback
    o.getListOrThrow❮C❯(path[, default]) -> List❮C❯ // default only for missing paths; other errors throw
    o.getStringList(path[, default]) -> StringList // read text values or return the supplied fallback
    o.getStringListOrThrow(path[, default]) -> StringList // default only for missing paths; other errors throw
    o.asStringList()/asStringListOrThrow() -> StringList // convert text values or a matching scalar
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
    o.addPlaceholderSource/removePlaceholderSource(source) // manage optional text placeholder sources
    o.addPlaceholderFilter/removePlaceholderFilter(filter) // manage optional text placeholder filters
    o.addPlaceholderEnvironmentSource([name]) // register the built-in environment source
    o.setPlaceholderVariableSource(variables[, name]) // register or update the built-in application-variable source
    o.addPlaceholderTextFilters() // register the built-in text filters

Validation Rule Construction Patterns
=====================================

.. code-block:: text

    o.createBranch(path) -> RulesBuilderBranch // establish a relative prefix without declaring rules
    o.parentPath() -> NamePath // complete prefix, empty on the main builder
    o.configureRoot([attributes]) // configure the implicit root or existing branch parent rule
    o.addRule(path, type[, attributes]) // add one named rule
    o.addAlternative(path, type[, attributes]) // add one permitted alternative at a path
    T(type) // EntryType requests an implicit scalar vr_entry on a ValueList rule
    T(constraints...) // EntryConstraints owns constraints applied after the entry type is known
    T(text) // EntryTitle and EntryDescription configure entry metadata
    o.setEntryType(type) // request the same entry through the RuleDefinition attribute interface
    o.addEntryAttributes(attributes) // queue owned attributes for the implicit scalar entry
    o.reset() // discard the current document; LogicFailure on branches
    o.takeRules() -> RulesPtr // validate, finalize, and reset the main builder; LogicFailure on branches
    T(mode, sources, targets[, error]) // create a dependency builder attribute
    o.apply(rule) // custom attributes mutate RuleDefinition without implementation types

Source and Trust Patterns
=========================

.. code-block:: text

    T::fromString/fromFile(input) -> SourcePtr // create a closed in-memory or file source
    o.resolve(context) -> SourceList // resolve an include into deterministic candidate sources
    o.check(source, context) -> AccessCheckResult // authorize one source independently
    o.validate(data) -> SignatureValidatorResult // validate exact parser-produced signature data
    o.sign(source, destination, signer) // insert or replace a document signature

Syntax Formatting Patterns
==========================

.. code-block:: text

    o.parseText(text[, identifier]) -> syntax::DocumentPtr // retain input errors as diagnostics
    o.format(DocumentPtr) -> FormatResult // retain damaged statements verbatim; require a non-null document
    o.tokens()/tokensForLine(line) -> List<TokenPtr> // borrow stored lists of shared tokens
    o.kind() -> syntax::TokenKind // lexical classification with raw enum access
    o.toText() -> String // standardized token name, including EndOfData and RegEx
    T::fromMultiLineOpen/Close(character) -> syntax::TokenKind // delimiter kind, otherwise EndOfData
    o.toValueType() -> ValueType // literal/opening delimiter category, otherwise Undefined
    o.isValue()/isPayload() -> bool // complete single-line literal or multiline content
    o.statements()/diagnostics()/lines() -> List<T> // borrow immutable snapshot collections
    o.firstToken()/endToken() -> ItemIndex // half-open statement boundaries in the document token list
    o.addPath(pattern) // add an alternative normalized component glob
    o.resolve(statement) -> FormatStyle // apply the per-property specificity cascade
