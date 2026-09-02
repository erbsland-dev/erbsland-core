..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Documents; Reference
    single: Text Rendering; Reference
    single: Text Document
    single: Text Node
    single: JSON
    single: JSON Value
    single: HTML Parser
    single: HtmlParser
    single: Layout Renderer
    single: Template Renderer

****************************
Text Documents and Rendering
****************************

Text Document
=============

Semantic Escaped Text
---------------------

Use :cpp:func:`TextNode::addEscapedText() <erbsland::text::TextNode::addEscapedText>` to insert untrusted text into a
document.
The method tolerantly decodes malformed input, groups ordinary characters into ``Text`` nodes, and creates one
indivisible ``EscapeSequence`` node for every escaped character.
This preserves wrapping and styling semantics while preventing raw control sequences from reaching a renderer.

JSON Values
===========

Value Tree
----------

``JsonValue`` is a copy-on-write value tree for JSON nulls, booleans, signed 64-bit integers, finite floating-point
numbers, Unicode strings, arrays, and objects.
``JsonArray`` and ``JsonObject`` use the regular Erbsland Core list and ordered string-map containers.
Copying a tree is inexpensive; the first mutation detaches the changed value.

Parsing and Formatting
----------------------

``fromStringOrThrow()`` strictly parses one complete RFC 8259 value and reports syntax, duplicate-key, encoding, and
configured-limit failures as :cpp:class:`ParseError <erbsland::err::ParseError>`.
``fromString()`` provides the optional-returning form.
Default limits accept 16 MiB documents, 64 container levels, one million values, and 8 MiB decoded strings.

``toString()`` produces deterministic compact JSON with object keys in ``StringMap`` order.
``JsonFormatOptions`` can enable space-indented output and non-ASCII escaping.

HTML Parser
===========

Introduction
------------

The HTML parser converts a tolerant subset of HTML fragments or documents into a
:cpp:class:`TextDocument <erbsland::text::TextDocument>`.
Malformed tags and entities are recovered as text where possible, so user-facing parsers can accept imperfect input.

Usage
-----

Create :cpp:class:`HtmlParser <erbsland::text::html::HtmlParser>` with an
:cpp:class:`AnyString <erbsland::text::AnyString>` compatible value and call ``parse()`` for the default
tolerant API.
Use ``parseOrThrow()`` when future unrecoverable parser errors should be reported as
:cpp:class:`ParseError <erbsland::err::ParseError>`.

Layout Rendering
================

The layout renderer compiles a deliberately small Jinja-like language into process-local bytecode and caches an
immutable compiled generation for each logical layout name.
The renderer supports exact source text, comments, explicit whitespace control, scalar expressions, conditionals,
lexically scoped list and ordered-map iteration, assignments, static includes, static inheritance, blocks, ``super()``
calls, and registered value filters.
Inserted scalar values are escaped automatically according to the logical layout-name suffix unless this behavior is
disabled in ``EnvironmentOptions``.

Setup and Thread Safety
-----------------------

Create an ``Environment``, add one or more loaders, and optionally enable automatic reload before the first render.
Loader, filter, and syntax setup is not thread-safe and becomes immutable at the first render.
``render()`` and ``setGlobalContext()`` are thread-safe.

Register application filters with ``addFilter()`` before the first render.
A ``FilterFn`` receives one immutable ``ValueList`` and returns one ``Value``.
Item zero is the resolved piped value; items one and two, when present, are the optional positional arguments in source
order.
The compiler evaluates those arguments once from left to right, while each application callback validates the list size
and all value types it accepts.
Filter callbacks can run concurrently and must provide their own synchronization for captured mutable state.
Filter names are ASCII identifiers.
Application filters can replace ordinary built-ins, but duplicate application registrations are rejected.
The names ``escape``, ``e``, and ``safe`` are reserved output modifiers.

The local ``Context`` supplied to ``render()`` takes precedence over the current global context snapshot.
Missing names and missing dotted members become null; null renders as empty text and is false.
Rendering a list or map directly is an error.

Loader Contract
---------------

A ``Loader`` receives a validated logical layout name and returns either one ``LayoutSource`` or ``std::nullopt``.
A source contains exact UTF-8 text, a diagnostic origin, and an opaque revision.
Higher-priority loaders are queried first; equal priorities retain addition order.

``FileSystemLoader`` accepts existing absolute directory roots.
It canonicalizes each root, maps layout components below it, rejects symbolic-link traversal below the root and
non-regular files, and reads strict UTF-8. Its revision changes with file content.

``ResourceLoader`` maps a logical name exactly to one compiled-resource key.
For identifier ``layouts`` and prefix ``application``, ``email/body.html`` resolves to
``(layouts, application/email/body.html)``.
No extension is added and the name is not made relative to the including layout.
The identifier is a portable resource token.
The optional prefix is empty or a normalized relative UTF-8 path with ``/`` separators and no empty, ``.`` or ``..``
components.

An explicit resource provider is retained through its shared pointer.
The convenience factory, or a null explicit provider, follows ``core::application().resources()`` and therefore the
application lifetime.
Do not create such an application-backed loader during unsafe static initialization.
Missing keys return ``std::nullopt`` so later loaders can participate.
Present data is transparently decompressed, must be valid UTF-8, and reports an error rather than being treated as
missing when decoding fails.
Resource origins use ``resource:<identifier>/<path>`` and revisions are derived from logical content.

The same layout tree can move from development to deployment without changing include names: use a ``FileSystemLoader``
rooted at ``<project>/data/layouts`` while developing, compile that tree as resources for deployment, then use
``ResourceLoader`` with its matching identifier and prefix.
For customization, register both and give the external filesystem loader higher priority.
Loader priority applies independently to the root and every included or inherited layout, so one external partial or
parent can override its embedded counterpart.

Without automatic reload, a layout is loaded and compiled once.
With reload enabled, every render recursively checks the current origin, revision, and dependency generations.
A successfully compiled source or dependency change replaces the affected cache generations.
An optional dependency appearing or disappearing also republishes its parents.
A failed change reports an error; renders already holding the previous immutable generation remain valid.

Supported Syntax
----------------

Raw text, comments, whitespace markers, and expression, statement, and comment delimiters retain the behavior described
for the running framework.
Closing delimiters inside quoted expression strings are treated as literal text.

Expressions
~~~~~~~~~~~

Expressions support:

- ASCII names and dotted lookup, such as ``name`` and ``user.profile.name``;
- signed decimal integers, finite decimal or scientific floats, ``true``, ``false``, and ``none``/``null``;
- single- and double-quoted UTF-8 strings;
- nested list literals and string-keyed map literals, including empty collections and one trailing comma;
- parentheses, unary ``+``, ``-``, and ``not``, binary ``+``, ``-``, ``*``, ``/``, ``~``, ``and``, and ``or``;
- ``==``, ``!=``, ``>``, ``>=``, ``<``, ``<=``, ``in``, ``not in``, ``is``, and ``is not``; and
- filter chains with zero, one, or two arguments, such as ``values | join(',')``.

String escapes are ``\\``, ``\'``, ``\"``, ``\b``, ``\f``, ``\n``, ``\r``, ``\t``, and ``\uXXXX``.
Unicode surrogate escapes must form a valid pair.
Unescaped control characters and invalid Unicode escapes are rejected.

From tightest to loosest binding, precedence is primary/member/collection expressions, filter chains, unary ``+`` and
``-``, ``*`` and ``/``, ``+``, ``-``, and ``~``, comparisons and membership/tests, ``not``, ``and``, then ``or``.
``and`` and ``or`` short-circuit and return the selected operand; ``not`` and comparisons return booleans.
Only one comparison is accepted in each comparison expression; combine multiple comparisons with ``and``.

Equality supports null, booleans, numbers, and text.
Integers and floats compare numerically without first converting the integer to a float.
Different scalar types compare unequal.
Ordering requires two numbers or two text values; text uses exact decoded-code-point order.
Comparing lists or maps is a runtime error.

Arithmetic accepts numbers only.
Integer ``+``, ``-``, ``*``, and ``/`` saturate to the signed 64-bit range using the math-domain saturating algorithms.
Integer division truncates toward zero and returns an integer.
Any floating-point operand selects double arithmetic and a floating-point result.
An integer or floating-point zero divisor, including negative zero, produces null.
The ``~`` operator converts null and scalars using renderer string conversion; containers are errors.

Membership supports text in text, a scalar in a list, and a text key in a map.
The argument-free tests are ``none`` /``null``, ``true``, ``false``, ``boolean``, ``integer``, ``float``, ``number``,
``text`` /``string``, ``list`` /``sequence``, ``map`` /``mapping``, ``iterable``, ``scalar``, ``even``, and ``odd``.
A test that does not apply to a value returns false.

Statements
~~~~~~~~~~

``if`` statements support nested ``elif`` and ``else`` branches and end with ``endif``.
Only conditions on the selected branch are evaluated.
``{% set name = expression %}`` stores a value in private render-local state without changing the supplied local or
global context.
A layout-level assignment is hoisted into that layout's setup program and runs before body output, regardless of its
source position.
Across inheritance, setup programs run once from the ultimate base to the most-derived layout, so derived assignments
override parent assignments.
Assignments nested in blocks, conditions, or loops retain their normal execution order and lexical scope.

Iteration
~~~~~~~~~

``{% for item in sequence %}`` iterates a list and ``{% for key, value in mapping %}`` iterates an ordered map.
The iterable expression is evaluated and callbacks are resolved exactly once when the loop starts.
One target requires a list; two distinct targets require a map.
Map entries follow ``StringMap`` key order.
Null, scalar values, mismatched target counts, duplicate targets, reserved keywords, and more than two targets are
strict errors.
An optional ``else`` branch runs only when the iterable produced no item.
The loop's private state and scope are removed before that branch executes.

Each iteration has a fresh lexical scope.
Its target names and assignments disappear before the next iteration, and assignments never update an outer binding.
Lookup proceeds through the innermost iteration scope, outer iteration scopes, render-local assignments, the caller's
local context, and finally the global context.
Nested loops temporarily shadow ``loop`` and restore the outer value when the inner loop ends.
The ``loop`` name cannot be an iteration or ``set`` target while a loop body is being compiled.

The immutable ``loop`` map contains:

- ``index`` and ``index0``: the one-based and zero-based current indexes;
- ``revindex`` and ``revindex0``: the one-based and zero-based remaining indexes;
- ``first`` and ``last``: booleans identifying the first and last item; and
- ``length``: the collection length.

Blocks and Inheritance
~~~~~~~~~~~~~~~~~~~~~~

``{% extends "layout/name" %}`` binds one exact static parent while compiling.
The statement accepts one string and no modifiers; dynamic expressions, fallback lists, relative resolution, and added
extensions are not supported.
An extending layout may contain only one ``extends`` statement, layout-level ``set`` statements, block declarations,
comments, and whitespace outside blocks.
Other output, expressions, includes, or control statements outside a block are syntax errors.

``{% block name %}`` declares an ASCII-identifier block and ends with ``{% endblock %}``.
Block names are unique within one layout, and a name after ``endblock`` is not supported.
Blocks can nest inside other blocks and structurally valid control bodies.
The renderer executes the ultimate base layout's body and dispatches each declaration to the most-derived available
implementation; an unoverridden declaration therefore renders its base fallback.
Dispatched blocks retain visible iteration scopes, ``loop`` metadata, render-local values, caller context, and global
context.

Inside a block, ``super()`` renders the next implementation in its resolved override chain and ``super.super()`` renders
the implementation after that.
A standalone ``{{ super() }}`` or ``{{ super.super() }}`` writes directly to the current output sink.
When a super call participates in a filter, grouping, comparison, logic expression, or ``set`` assignment, its output is
first captured as text and then processed as a normal value.
Arguments, deeper chaining, and calls outside blocks are syntax errors; requesting a depth with no implementation is a
runtime error.

Includes
~~~~~~~~

``{% include "layout/name" %}`` binds one exact static logical layout name while compiling.
Dynamic expressions, relative names, fallback lists, and added extensions are not supported.
``ignore missing`` and either ``with context`` or ``without context`` may follow the name in either order, but each
modifier is accepted at most once.
The default is ``with context``.

With context, an included layout sees the immediate parent's active iteration scopes, private assignments, caller
context, and global context.
Assignments made by the child remain private to that child.
Without context, the child sees only the global context and its own private assignments.
A nested include always inherits from its immediate parent according to its own context modifier.

``ignore missing`` suppresses only a missing-layout error.
Invalid names, loader and resource failures, invalid UTF-8, syntax and runtime errors, cycles, and limits remain
visible.
A compiled graph can traverse at most the configured static-dependency depth below the root; the default is 32 edges.
Direct, indirect, and mixed include/inheritance cycles are syntax errors.

Compiled generations retain their resolved immutable parents, block chains, include dependencies, and explicit empty
descriptors for ignored missing layouts.
Nested programs append to one shared output sink and the renderer joins the final output once.

Filters are resolved while compiling a layout, so an unknown filter is a syntax error.
Filter chains run from left to right and apply normal callback resolution before and after each filter invocation.
Arguments are evaluated once from left to right.
A third argument is rejected while compiling.

The ordinary built-ins are:

- text: ``capitalize``, ``lower``, ``upper``, ``trim([characters])``, and ``replace(old, new)``;
- collections: ``first``, ``last``, ``join([separator [, member]])``, ``length``/``count``, ``reverse``,
  ``sort([reverse [, case_sensitive]])``, ``keys``, ``values``, and ``items``;
- numeric: ``abs``, ``round([precision [, common|ceil|floor]])``, ``sum([start])``, ``min``, and ``max``; and
- general/output: ``default``/``d([replacement [, use_false]])`` and ``tojson([indent])``.

Empty ``first``, ``last``, ``min``, and ``max`` produce null.
``tojson`` accepts indentation from zero through 16 and returns ordinary unsafe text; use ``|tojson|safe`` for a raw
JSON insertion.

Escaping
~~~~~~~~

Automatic escaping is enabled by default.
``.html``, ``.xml``, ``.elcl``, ``.md``, and ``.json`` select ``Html``, ``Xml``, ``Config``, ``Markdown``, and ``Json``
respectively.
Custom bounded suffix mappings use longest-match selection; unmatched names use ``None``.
The logical name of the program-owning layout selects the format, including included layouts and inherited block
implementations.

All directly emitted scalars pass through ``Value::toString().toEscaped(selectedFormat)``.
Null remains empty and containers remain non-renderable.
``safe`` suppresses automatic escaping for one insertion.
``escape`` and its Jinja compatibility alias ``e`` force escaping, with an optional canonical format argument.
With no argument they use the suffix-selected format or HTML when no format was selected.
These modifiers are accepted only as the final filter of a direct output expression; they are rejected in conditions,
assignments, arguments, and expressions involving ``super()``.
Disabling automatic escaping does not disable explicit ``escape``.

Direct block output and direct ``super()`` rendering are already layout-safe.
Capturing and transforming super output turns it into ordinary unsafe text.
Markdown escaping follows the `CommonMark backslash-escape rules <https://spec.commonmark.org/spec#backslash-escapes>`_
for normal Markdown text; it does not protect contexts in which CommonMark disables backslash escaping.

Limits
~~~~~~

``EnvironmentOptions::renderLimits()`` configures positive limits for total generated and captured output, executed
instructions, runtime nesting, callback resolution, lexical scopes, call frames, value-stack depth, and static
dependency depth.
Defaults are 64 MiB, 10,000,000 instructions, 128 nesting levels, 32 callback levels, 128 scopes, 128 call frames, 1,024
stack values, and 32 static dependency edges.
Exceeding a limit raises a ``Limit`` render error.

Arbitrary function calls, assignments to dotted names, recursive loops, block capture, dynamic dependencies, macros, and
persisted bytecode remain unsupported.
A whitespace marker removes adjacent ASCII whitespace; no other source whitespace is changed.

Layout names use lowercase ASCII letters, digits, ``-``, ``_``, ``.``, and ``/``.
They are relative, contain no empty, ``.`` or ``..`` component, and contain at most 200 characters.

Errors
------

``RenderError`` reports a category, title, description, logical layout, origin, source location, optional source
snippet, and nested render frames.
A nested failure preserves the leaf layout, origin, and location and records outer include or inheritance call sites in
outermost-first order as one-based ``layout:line:column`` frames.
Loading and callback exceptions are kept as nested causes.
Invalid supported operations are strict; only missing value lookup is silent.

Interface
=========

.. doxygenstruct:: erbsland::text::CodeSnippet
    :members:
.. doxygenclass:: erbsland::text::CodeSnippetMarker
    :members:

.. doxygentypedef:: erbsland::text::CodeSnippetMarkerList
.. doxygenclass:: erbsland::text::html::HtmlParser
    :members:
.. doxygentypedef:: erbsland::text::json::JsonArray

.. doxygentypedef:: erbsland::text::json::JsonObject
.. doxygenclass:: erbsland::text::json::JsonFormatOptions
    :members:
.. doxygenclass:: erbsland::text::json::JsonParseOptions
    :members:
.. doxygenenum:: erbsland::text::json::JsonType
.. doxygenclass:: erbsland::text::json::JsonValue
    :members:
.. doxygenclass:: erbsland::text::PlainTextRenderer
    :members:
.. doxygenclass:: erbsland::text::render::Context
    :members:
.. doxygenclass:: erbsland::text::render::Delimiters
    :members:
.. doxygenclass:: erbsland::text::render::Environment
    :members:
.. doxygenclass:: erbsland::text::render::EnvironmentOptions
    :members:
.. doxygenclass:: erbsland::text::render::FileSystemLoader
    :members:
.. doxygenclass:: erbsland::text::render::FileSystemLoaderOptions
    :members:
.. doxygenclass:: erbsland::text::render::LayoutSource
    :members:
.. doxygenclass:: erbsland::text::render::Loader
    :members:
.. doxygenclass:: erbsland::text::render::RenderError
    :members:
.. doxygenenum:: erbsland::text::render::RenderErrorCategory
.. doxygenclass:: erbsland::text::render::RenderErrorContext
    :members:
.. doxygenclass:: erbsland::text::render::RenderLimits
    :members:
.. doxygenclass:: erbsland::text::render::ResourceLoader
    :members:
.. doxygenclass:: erbsland::text::render::Value
    :members:

.. doxygentypedef:: erbsland::text::render::ValueList

.. doxygentypedef:: erbsland::text::render::ValueMap

.. doxygentypedef:: erbsland::text::render::ValueCallbackFn

.. doxygentypedef:: erbsland::text::render::FilterFn
.. doxygenenum:: erbsland::text::render::ValueType
.. doxygenclass:: erbsland::text::TextDocument
    :members:
.. doxygenclass:: erbsland::text::TextNode
    :members:
.. doxygenclass:: erbsland::text::TextNodeData
    :members:
.. doxygenclass:: erbsland::text::TextNodeType
    :members:
.. doxygenclass:: erbsland::text::TextWalkResult
    :members:
.. doxygenenum:: erbsland::text::TextWalkStatus
