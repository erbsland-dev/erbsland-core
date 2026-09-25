..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration Language; Reference
    single: Configuration Parsing
    single: Configuration Documents
    single: Configuration Sources
    single: Configuration Source Resolution
    single: Configuration Access Control
    single: Configuration Names
    single: Configuration Locations
    single: Configuration Scalar Data
    single: Configuration Values
    single: Configuration Signing
    single: Configuration Signature Validation
    single: Configuration Errors

**********************
Configuration Language
**********************

Configuration Parsing
=====================

:cpp:class:`erbsland::conf::Parser <erbsland::conf::Parser>` reads one closed source and returns a typed
:cpp:class:`erbsland::conf::Document <erbsland::conf::Document>`. Each parser instance is reentrant and can be used
independently in one thread.
Use a separate instance in each concurrently executing thread.

Parsing Files and Text
----------------------

The convenience methods cover the common file and in-memory cases:

.. code-block:: cpp

    using namespace el::text::literals;

    el::conf::Parser parser;
    auto fromFile = parser.parseFileOrThrow(el::path::Path{"settings.elcl"_el});
    auto fromText = parser.parseTextOrThrow("[server]\nport = 8443\n"_el);

The equivalent source-level API is useful when an application implements its own source:

.. code-block:: cpp

    auto source = el::conf::Source::fromString("enabled = yes\n"_el);
    auto document = parser.parseOrThrow(source);

``parseOrThrow`` reports every failure as ``erbsland::conf::ConfError``.
``parse`` returns a null document instead and stores its
:cpp:class:`erbsland::conf::ConfErrorContext <erbsland::conf::ConfErrorContext>` for
:cpp:func:`erbsland::conf::Parser::lastError <erbsland::conf::Parser::lastError>`.
Regular-expression literals are retained without compilation, so invalid pattern syntax is reported later as
:cpp:class:`re::RegExError <erbsland::re::RegExError>` when the expression is compiled or first used.

Customization
-------------

Includes are resolved by a :cpp:class:`erbsland::conf::SourceResolver <erbsland::conf::SourceResolver>` and approved by
an :cpp:class:`erbsland::conf::AccessCheck <erbsland::conf::AccessCheck>`.
The defaults implement restricted file-based includes.
A :cpp:class:`erbsland::conf::SignatureValidator <erbsland::conf::SignatureValidator>` can be installed for signed
documents.
Set these collaborators before starting a parse; null resolver or access-check values disable include handling.

Placeholder Extension
---------------------

The parser can optionally expand ``${source:parameter}`` expressions in ordinary quoted text values.
It uses the shared providers from :doc:`/reference/text/placeholders`.
Applications add implementations of
:cpp:class:`erbsland::text::placeholder::Source <erbsland::text::placeholder::Source>` with
:cpp:func:`erbsland::conf::Parser::addPlaceholderSource <erbsland::conf::Parser::addPlaceholderSource>` and optional
:cpp:class:`erbsland::text::placeholder::Filter <erbsland::text::placeholder::Filter>` implementations with
:cpp:func:`erbsland::conf::Parser::addPlaceholderFilter <erbsland::conf::Parser::addPlaceholderFilter>`.

Source and filter names are normalized as regular ELCL names and must be unique within a parser.
Providers may implement several names.
Their parameters are escape-decoded but retain case, while the names passed to callbacks are normalized.
Expansion is inactive when no source is registered, even if filters are present.

:cpp:func:`erbsland::conf::Parser::addPlaceholderEnvironmentSource
<erbsland::conf::Parser::addPlaceholderEnvironmentSource>` registers the built-in ``env`` source under its default or a
custom name.
:cpp:func:`erbsland::conf::Parser::setPlaceholderVariableSource
<erbsland::conf::Parser::setPlaceholderVariableSource>` registers or updates the built-in ``var`` source under its
default or a custom name.
:cpp:func:`erbsland::conf::Parser::addPlaceholderTextFilters
<erbsland::conf::Parser::addPlaceholderTextFilters>` registers the built-in text filters.
See
:doc:`/topics/conf/placeholders`, :doc:`/topics/text_placeholders/built_in_sources`, and
:doc:`/topics/text_placeholders/built_in_filters` for the syntax and behavior.

Configuration Documents
=======================

A :cpp:class:`erbsland::conf::Document <erbsland::conf::Document>` is the root of a parsed configuration value tree.
It provides the complete :cpp:class:`erbsland::conf::Value <erbsland::conf::Value>` interface and can also produce a
flat map from absolute name paths to values.
Values retain their source location, allowing errors and diagnostics to point back to the input after parsing has
finished.
An implicit parent such as ``server`` in ``[server.tls]`` is internally an intermediate section.
The public :cpp:func:`erbsland::conf::Value::isSectionWithNames <erbsland::conf::Value::isSectionWithNames>` and
:cpp:func:`erbsland::conf::Value::getSectionWithNames <erbsland::conf::Value::getSectionWithNames>` methods treat it as
a regular named section.

Programmatic Construction
-------------------------

:cpp:class:`erbsland::conf::DocumentBuilder <erbsland::conf::DocumentBuilder>` creates the same tree structure
without parsing text.
Sections must be introduced before values are added to them; intermediate section maps are created where required.
The builder rejects name collisions and invalid document structures with a syntax error.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    erbsland::conf::DocumentBuilder builder;
    builder.addSectionMap("server"_el);
    builder.addText("name"_el, "gateway"_el);
    builder.addInteger("port"_el, 8443);
    auto document = builder.getDocumentAndReset();

Single names are relative to the most recently added section.
A multi-component name path selects its section explicitly.
Index and text-index components cannot be used by this builder.
Scalar overloads accept the same Core types returned by ``Value``.
Regular expressions must be compiled before they are added, and a null ``re::RegExPtr`` is rejected.

Configuration Sources
=====================

A :cpp:class:`erbsland::conf::Source <erbsland::conf::Source>` supplies UTF-8 configuration text line by line.
A source object is deliberately lightweight and closed when created.
The parser opens it once, reads until the end, then closes it and releases any external resources.
Returned lines are owning Core strings and remain valid after subsequent reads.
Sources also provide best-effort excerpts for diagnostics.
In-memory sources scan only the requested area, while stream sources retain the five most recently read lines and may
read up to two immediately following context lines when an error excerpt is requested.

Built-in Sources
----------------

Use :cpp:func:`erbsland::conf::Source::fromFile <erbsland::conf::Source::fromFile>` for a lazy file source and
:cpp:func:`erbsland::conf::Source::fromString <erbsland::conf::Source::fromString>` for in-memory text:

.. code-block:: cpp

    using namespace el::text::literals;

    auto fileSource = el::conf::Source::fromFile(erbsland::path::Path{"settings.elcl"_el});
    auto textSource = el::conf::Source::fromString("answer = 42\n"_el);

    el::conf::Parser parser;
    auto document = parser.parseOrThrow(textSource);

Custom sources should defer I/O and heavy allocation to ``open()``, report I/O failures as configuration errors, include
newline sequences in returned lines, and make an empty line value signal the end of input.
Each source has a stable :cpp:class:`erbsland::conf::SourceIdentifier <erbsland::conf::SourceIdentifier>` for locations
and access decisions.

Configuration Source Resolution
===============================

Include directives describe a source to load relative to the source containing the directive.
A
:cpp:class:`erbsland::conf::SourceResolver <erbsland::conf::SourceResolver>` receives this context and returns one or
more closed source objects in deterministic order.
It resolves names only; each result is still passed through the configured access check before the parser opens it.

The default :cpp:class:`erbsland::conf::FileSourceResolver <erbsland::conf::FileSourceResolver>` implements file
includes, including supported patterns and recursive requests.
Relative paths are based on the including file.
Its results work with :cpp:class:`erbsland::conf::FileAccessCheck <erbsland::conf::FileAccessCheck>`, which applies the
security boundary independently.

Applications can install another resolver with
:cpp:func:`erbsland::conf::Parser::setSourceResolver <erbsland::conf::Parser::setSourceResolver>`, for example to
address embedded resources or a database.
A custom resolver should create lightweight sources and preserve a stable source identifier; it should not open or parse
them itself.

Configuration Access Control
============================

Every source requested by the parser is passed to an
:cpp:class:`erbsland::conf::AccessCheck <erbsland::conf::AccessCheck>` before it is opened.
This includes the initial source.
The check receives both the requested source and its parent source, when the request originated from an include
directive.

The default :cpp:class:`erbsland::conf::FileAccessCheck <erbsland::conf::FileAccessCheck>` accepts ordinary ELCL files
in the directory of the initial file and, by default, its subdirectories.
This prevents an include from silently escaping the configuration tree.
Applications that load configurations from another trust boundary can install their own check with
:cpp:func:`erbsland::conf::Parser::setAccessCheck <erbsland::conf::Parser::setAccessCheck>`.

A custom check returns the appropriate access result for each request.
It may also throw a configuration ``erbsland::conf::ConfError`` when it needs to report a more specific reason.
Setting a null access check disables includes together with source resolution; it is not a way to bypass access checks.

Configuration Names
===================

ELCL addresses values with typed names.
A regular name selects a named child, an index selects an element by its position, and text and text-index names
represent the corresponding language forms.
:cpp:class:`erbsland::conf::Name <erbsland::conf::Name>` stores one such component without losing its type.

A :cpp:class:`erbsland::conf::NamePath <erbsland::conf::NamePath>` is an ordered sequence of components.
It is used for document lookup, validation rules, errors, and flat document maps.
Builder APIs accept
:cpp:type:`erbsland::conf::NamePathLike <erbsland::conf::NamePathLike>`, allowing a single Core string, a
:cpp:class:`erbsland::conf::Name <erbsland::conf::Name>`, or a complete path where appropriate. Use
:cpp:func:`erbsland::conf::toNamePath <erbsland::conf::toNamePath>` when an owning path is needed explicitly.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    const auto section = erbsland::conf::toNamePath("server"_el);
    const auto value = document->value(section);

Configuration Locations
=======================

A :cpp:class:`erbsland::unit::CodeLocation <erbsland::unit::CodeLocation>` stores zero-based line, column, and absolute
code-point indices.
Its textual representation uses customary one-based numbers for display.
A
:cpp:class:`erbsland::conf::Location <erbsland::conf::Location>` combines a code location with a
:cpp:class:`erbsland::conf::SourceIdentifier <erbsland::conf::SourceIdentifier>`, so locations remain meaningful
after the source has been closed.

Source identifiers contain a source name and path.
The name describes the source kind or protocol, while the path identifies the concrete input.
Their textual form is suitable for diagnostics, but callers should compare the structured identifier when identity
matters.

Configuration Scalar Data
=========================

Configuration scalar values use the corresponding Core types directly:

*   Text uses :cpp:type:`erbsland::text::String <erbsland::text::String>`.
*   Bytes use :cpp:class:`erbsland::mem::ByteBlock <erbsland::mem::ByteBlock>`.
*   Dates, times, date-times, and deltas use :cpp:class:`erbsland::time::Date <erbsland::time::Date>`,
    :cpp:class:`erbsland::time::Time <erbsland::time::Time>` or
    ``erbsland::time::TimeWithZone``,
    :cpp:class:`erbsland::time::DateTime <erbsland::time::DateTime>`, and
    :cpp:class:`erbsland::time::CalendarDelta <erbsland::time::CalendarDelta>`.
*   Regular expressions use the immutable shared pointer
    :cpp:type:`erbsland::re::RegExPtr <erbsland::re::RegExPtr>`.

Regular-expression literals are stored with lazy compilation.
Parsing retains their pattern, flags, and settings without invoking the regular-expression compiler.
The first matching operation compiles the expression and may throw
:cpp:class:`re::RegExError <erbsland::re::RegExError>` for an invalid pattern.
Call :cpp:func:`re::RegEx::compileNow() <erbsland::re::RegEx::compileNow>` when an application needs explicit validation
before using configuration values.

Standalone ELCL times without a suffix are stored as floating :cpp:class:`Time <erbsland::time::Time>` values.
Values with ``Z`` or a numeric offset are stored as ``TimeWithZone``.
Both use :cpp:enumerator:`ValueType::Time <erbsland::conf::ValueType::Time>`.
The ``asTime()`` family removes a zone, while the ``asTimeWithZone()`` family attaches ``TimeZone::local()`` to a
floating value.

Date-times without a suffix are interpreted in the system-local zone at the complete civil date and time.
This preserves historical shifts, daylight-saving state, gaps, and folds in the resulting UTC-backed ``DateTime``.
Every ELCL delta literal is stored as one independent ``CalendarDelta`` component, including month and year values.
Ordinary ELCL value lists remain lists and are not implicitly combined into one delta.

Integers and floating-point values use the :cpp:type:`erbsland::conf::Integer <erbsland::conf::Integer>` and
:cpp:type:`erbsland::conf::Float <erbsland::conf::Float>` aliases.
Validation rules use ``erbsland::text::CaseSensitivity`` to define their text matching mode.

Lists and matrices are structural value types rather than scalar classes.
Access them through the typed operations of :cpp:class:`erbsland::conf::Value <erbsland::conf::Value>`.

Configuration Values
====================

Text values are exposed as :cpp:type:`erbsland::text::String <erbsland::text::String>`, and lists of text use
:cpp:type:`erbsland::text::StringList <erbsland::text::StringList>`. Read-only strings are returned by value so
callers can keep inexpensive, independent references to their content.

A :cpp:class:`erbsland::conf::Value <erbsland::conf::Value>` is one node in a configuration tree.
Its value type distinguishes scalar values, value lists and matrices, sections, section lists, and sections with text
indexes.
Typed accessors verify this type and return the corresponding scalar or child structure.
Navigation accepts names and name paths, while iteration exposes children without flattening the tree.

The configuration scalar mapping is deliberately shared with the other Core domains: bytes use ``mem::ByteBlock``;
dates, times, date-times, and deltas use the matching ``time`` types; and regular expressions use ``re::RegExPtr``.
Regular-expression literals are compiled lazily, and copies share the same immutable compilation state.
The non-throwing accessors return an empty byte block, invalid date or date-time, midnight, a zero delta, or ``nullptr``
when the requested value is missing or has another type.
The ``...OrThrow()`` variants report a type mismatch instead.

Values also retain their name, absolute name path, location, and validation metadata.
Shared pointers express the ownership contract: callers may keep a value after discarding the parser or document handle,
and const pointers provide read-only traversal.
Core strings use copy-on-write ownership rather than borrowed views.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    const auto server = document->value("server"_el);
    const auto port = server->value("port"_el)->asInteger();
    const erbsland::text::String name = server->value("name"_el)->asText();

Use :cpp:func:`erbsland::conf::Document::toFlatValueMap <erbsland::conf::Document::toFlatValueMap>` when a complete
absolute-path index is more convenient than tree traversal.

Configuration Signing
=====================

Signing APIs use :cpp:class:`erbsland::path::Path <erbsland::path::Path>` for source and destination files and Core
strings for textual signature data.
:cpp:class:`erbsland::conf::Signer <erbsland::conf::Signer>` reads the complete source, validates its UTF-8 encoding and
line limits, calculates the document digest, and asks a user-supplied
:cpp:class:`erbsland::conf::SignatureSigner <erbsland::conf::SignatureSigner>` to create the signature text.

The output is a copy of the source with an initial signature line inserted or replaced.
The signer preserves the document's line-ending convention and the byte-level digest rules used by the parser.
It does not validate ELCL syntax, so applications should parse a document successfully before signing it.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    auto implementation = std::make_shared<MySignatureSigner>();
    erbsland::conf::Signer signer{implementation};
    signer.sign(
        erbsland::path::Path{"settings.elcl"_el},
        erbsland::path::Path{"settings.signed.elcl"_el},
        "Release Service"_el);

Configuration Signature Validation
==================================

When a document contains signature metadata, the parser calculates the document digest and passes the signature, digest,
signing-person text, and source information to the configured
:cpp:class:`erbsland::conf::SignatureValidator <erbsland::conf::SignatureValidator>`.
The validator is responsible for interpreting the signature text and checking it against the application's trust policy.

No validator is installed by default.
Unsigned documents can then be parsed, while signed documents are rejected because their authenticity cannot be
established.
Install a validator with
:cpp:func:`erbsland::conf::Parser::setSignatureValidator <erbsland::conf::Parser::setSignatureValidator>` before parsing signed input.

Validation is performed on the exact signature data produced by the parser.
Implementations must not normalize digest or signature bytes, and should return the defined validation result or throw a
configuration error when the failure needs additional diagnostic context.

Configuration Errors
====================

Configuration diagnostics use :cpp:type:`erbsland::text::String <erbsland::text::String>` for messages and
:cpp:class:`erbsland::path::Path <erbsland::path::Path>` for affected files. A
``erbsland::conf::ConfErrorContext`` combines a short title, a detailed description, a configuration error category, and
any available code location, name path, file path, and source excerpt.
``erbsland::conf::ConfError`` carries this context through the Core exception and diagnostic interfaces.

The exception reason and ``what()`` text are the short title.
Use the diagnostic interface when presenting an error to a user; its structured text document contains the description
and every available context field.
Path, stream, and decoding failures remain attached as causes and are rendered by
:cpp:class:`erbsland::err::DiagnosticHelper <erbsland::err::DiagnosticHelper>`.
The diagnostic's ``toString()`` method renders the same text document through
:cpp:class:`erbsland::text::PlainTextRenderer <erbsland::text::PlainTextRenderer>`. Structured and styled consumers
can render that document without maintaining a separate diagnostic representation.

The throwing parser entry points propagate this structured error.
The non-throwing entry points return a null document and retain the same error in
:cpp:func:`erbsland::conf::Parser::lastError <erbsland::conf::Parser::lastError>`.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    erbsland::conf::Parser parser;
    auto document = parser.parseText("value: ?"_el);
    if (document == nullptr) {
        const auto context = parser.lastError();
        auto error = erbsland::conf::ConfError{context};
        // Present error.diagnostic() to the user.
    }

Error categories distinguish syntax, encoding, I/O, limits, signatures, validation, and internal failures.
Code should normally preserve the original category when adding context around an error.

Interface
=========

.. doxygenclass:: erbsland::conf::AccessCheck
    :members:

.. doxygentypedef:: erbsland::conf::AccessCheckPtr
.. doxygenenum:: erbsland::conf::AccessCheckResult
.. doxygenstruct:: erbsland::conf::AccessSources
    :members:
.. doxygenclass:: erbsland::conf::ConfError
    :members:
.. doxygenclass:: erbsland::conf::ConfErrorCategory
    :members:
.. doxygenclass:: erbsland::conf::ConfErrorContext
    :members:
.. doxygenclass:: erbsland::conf::Document
    :members:

.. doxygentypedef:: erbsland::conf::DocumentPtr
.. doxygenclass:: erbsland::conf::DocumentBuilder
    :members:
.. doxygenclass:: erbsland::conf::FileAccessCheck
    :members:

.. doxygentypedef:: erbsland::conf::FileAccessCheckPtr
.. doxygenclass:: erbsland::conf::FileSourceResolver
    :members:

.. doxygentypedef:: erbsland::conf::FileSourceResolverPtr
.. doxygentypedef:: erbsland::conf::Float
.. doxygentypedef:: erbsland::conf::Integer
.. doxygenclass:: erbsland::conf::Location
    :members:
.. doxygenclass:: erbsland::conf::Matrix
    :members:
.. doxygenclass:: erbsland::conf::Name
    :members:
.. doxygenclass:: erbsland::conf::NamePath
    :members:

.. doxygentypedef:: erbsland::conf::NamePathLike

.. doxygentypedef:: erbsland::conf::NamePathList

.. doxygenfunction:: erbsland::conf::toNamePath(const NamePathLike &namePathLike) -> NamePath
.. doxygenenum:: erbsland::conf::NameType

.. doxygenfunction:: erbsland::conf::toString(const NameType nameType) noexcept -> text::String
.. doxygenclass:: erbsland::conf::Parser
    :members:
.. doxygenclass:: erbsland::conf::SignatureSigner
    :members:

.. doxygentypedef:: erbsland::conf::SignatureSignerPtr
.. doxygenstruct:: erbsland::conf::SignatureSignerData
    :members:
.. doxygenclass:: erbsland::conf::SignatureValidator
    :members:

.. doxygentypedef:: erbsland::conf::SignatureValidatorPtr
.. doxygenstruct:: erbsland::conf::SignatureValidatorData
    :members:
.. doxygenenum:: erbsland::conf::SignatureValidatorResult
.. doxygenclass:: erbsland::conf::Signer
    :members:
.. doxygenclass:: erbsland::conf::Source
    :members:
.. doxygenclass:: erbsland::conf::SourceIdentifier
    :members:
.. doxygenclass:: erbsland::conf::SourceResolver
    :members:

.. doxygentypedef:: erbsland::conf::SourceResolverPtr
.. doxygenstruct:: erbsland::conf::SourceResolverContext
    :members:
.. doxygenclass:: erbsland::conf::TestFormat
    :members:
.. doxygenclass:: erbsland::conf::Value
    :members:
.. doxygenclass:: erbsland::conf::ValueIterator
    :members:
.. doxygenclass:: erbsland::conf::ValueType
    :members:
