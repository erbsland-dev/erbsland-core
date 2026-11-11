.. index::
    single: ELCL Syntax and Formatting
    single: Syntax Highlighting; Configuration

**************************
ELCL Syntax and Formatting
**************************

Source Snapshots
================

Include ``<erbsland/all_conf_syntax.hpp>`` explicitly.
The ``conf::syntax`` namespace is not included by ``all_conf.hpp`` and is not flattened into the root namespace.

``syntax::Parser::parseText()`` returns a ``syntax::DocumentPtr`` owning a lossless, immutable snapshot.
Unlike ``conf::Parser``, it retains comments, spacing, spelling, delimiters and erroneous source.
Includes, placeholders, signatures and embedded languages are not evaluated.
Tokens partition the original text and expose half-open byte ranges and zero-based source locations.
The token, statement, diagnostic and line accessors return const references to Core ``List`` collections.
Tokens use shared ``syntax::TokenPtr`` ownership; retaining a token or copying its list keeps the tokens alive after the
document is released.
``tokensForLine()`` supports line-oriented highlighting by returning a const reference to a stored list of the same
token pointers, without constructing a list on each lookup.
Invalid line indices return a shared empty list.
Borrowed list references remain valid for the lifetime of the document.
Statement token boundaries use ``ItemIndex``.

``TokenKind`` is the shared classification used by the lexer and syntax snapshots, and exposes its enum through
``raw()``.
It preserves the lexer names, including ``EndOfData`` and ``RegEx``; ``toText()`` returns their standardized diagnostic
spelling.
Default construction produces ``Error``.
``fromMultiLineOpen()`` and ``fromMultiLineClose()`` classify a repeated delimiter character, returning ``EndOfData``
for an unrecognized character.
``SectionName`` and ``SectionTextName`` identify names in section declarations after syntax context is resolved.
``toValueType()`` maps complete literals and multiline opening delimiters to their semantic value category.
Other tokens, including multiline payload, return ``ValueType::Undefined``.
``isValue()`` recognizes complete single-line literals; ``isPayload()`` recognizes multiline content tokens.

.. code-block:: cpp

    #include <erbsland/all_conf_syntax.hpp>

    auto document = el::conf::syntax::Parser{}.parseText(source);
    auto result = el::conf::syntax::Formatter{}.format(document);
    auto formatted = result.text();

    for (const auto &token : document->tokensForLine(el::LineIndex{0})) {
        auto originalText = token->text();
    }

Parsing empty or malformed input still returns a non-null document.
The formatter accepts the shared document directly and rejects null pointers with ``ParameterFailure``.
Formatting diagnostics are also exposed through a const ``List`` reference.

Diagnostics refer to the original input.
Reparse formatted output to obtain updated highlighting coordinates.
No renderer or incremental edit protocol is imposed on consumers.

Recovery
========

Expected input errors become diagnostics.
Scanning resumes on a physical line boundary, preserving recognized tokens and the failed line suffix.
Multiline values retain their delimiter and indentation context after a malformed line.
Syntax recognition uses one decoder and character stream for the complete snapshot, sharing lexical productions with
strict configuration parsing.
Recovery happens within those productions, where continued-value context is available.
An unindented statement ends a broken block and is processed again as a new statement.

The formatter preserves a damaged statement verbatim, including its multiline content, and formats valid surrounding
statements.
It does not invent missing delimiters or fix application-level configuration errors.
Internal failures are not treated as source errors.

Statement name paths are resolved locally.
Relative sections use the last absolute section; ``@include`` resets that context without opening another source.
A malformed section invalidates the current section context.
Values with no resolved path cannot match path selectors.
Repeated list sections use their declaration path without occurrence indices.

Styles and Selectors
====================

``Format`` contains a complete base ``FormatStyle`` and ordered ``FormatRule`` objects.
Each rule combines a ``Selector`` with a ``FormatStylePatch``.
Only explicitly set patch properties participate in the cascade.

Selectors combine their conditions with AND.
Their path alternatives combine with OR.
Supported conditions select statement kind, relative/list sections, declared section-name count greater than a
threshold, meta assignments and value types.
A section match does not implicitly select its values.

Name paths use component globs, sharing the filesystem path matcher.
``*`` stays inside one name; a complete ``**`` component matches zero or more names.
``server.**.port`` matches both ``server.port`` and ``server.http.port``.
Quoted components preserve text-name identity and may contain literal dots.
Regular names use ELCL normalization.
Embedded recursive stars and runs of three stars are invalid.

Each property independently takes the most specific matching rule.
Exact paths outrank globs, which outrank selectors without paths.
Globs rank by literal component count, literal character count, then fewer recursive and ordinary wildcards.
Further ties use the number of other conditions, then insertion order: later rules win.
Only matching path alternatives contribute specificity.

Formatting Options
==================

Regular section and value names default to Title Case with spaces.
Each has independent case and separator settings.
Quoted names and identifiers inside text values default to ``Keep``.
Enabling their normalization changes only text that parses completely as one regular identifier, but can change
case-sensitive names or application text.
Selectors match the original names; such changes can affect selector matching on a later pass.

``abbreviations()`` supplies whole identifier words that remain uppercase during title casing.
The defaults are HTTP, HTTPS, FTP, FTPS, SSH, and VR.
Matching ignores ASCII case and uses spaces and underscores as word boundaries: ``http_server`` becomes ``HTTP Server``,
while ``http2`` becomes ``Http2`` unless HTTP2 is also configured.
The list also applies to opted-in quoted names and text identifiers.
Lower, Upper, and Keep case settings retain their usual behavior.
``setAbbreviations()`` accepts single regular identifier words, normalizes them to uppercase, and removes duplicates.
An empty list disables abbreviation handling; a style patch replaces the inherited list in full.

``booleanCase()`` selects Keep, Lower, Title, or Upper independently of ``booleanForm()``.
The default is Title case with the original word pair retained: ``YES`` becomes ``Yes``, and ``off`` becomes ``Off``.
``BooleanForm`` selects Keep, TrueFalse, YesNo, EnabledDisabled, or OnOff.
With Keep case, literals already using the requested pair preserve their exact spelling; changing pairs produces
lowercase spelling.
For example, selecting YesNo with Keep case preserves ``YES`` and changes ``ON`` to ``yes``.
Boolean formatting preserves truth values and applies in scalars, vectors, and matrices.
Wrapping and alignment use the formatted spelling.
Quoted text, comments, multiline payloads, and damaged statements are not rewritten as Boolean literals.

Section indentation is the zero-based opening-bracket column.
Finite prefixes are filled with dashes, reserving the required opening list star.
Infinite indentation right-aligns the header.
A finite suffix counts decoration characters, including the optional closing star; infinite suffix fills to the width.
Both infinite center the header, placing an odd spare dash on the right.
Inline comments reserve their own space.
Header content is never truncated.

``valueWrapping()`` controls placement of complete non-multiline values without splitting their contents:

* ``Keep`` retains whether a value starts on the assignment line or a continuation line.
* ``SameLine`` joins the value onto its assignment line, even beyond the soft width limit.
* ``NextLine`` places the value on an indented continuation line.
* ``NextLineIfFits`` moves an overlong assignment only when the complete continuation line fits the limit (the default).
* ``WhenTooLong`` moves overlong assignments even when the continuation still exceeds the limit.

Trailing value comments move with their value and participate in the width calculation.
Blank lines and comments between the separator and its value prevent reflow.
Multiline text, code, regular expressions, byte payloads, matrices, and bulleted lists retain their structural layout.
Style patches can override value placement independently of the other properties.

Vectors support ``Keep``, ``Compact`` and ``Multiline``.
Compact layout uses one comma-separated assignment if it fits, otherwise one value per continued line.
Matrices retain rows and support ``Keep`` or ``Align``.
Alignment falls back to minimal spacing when aligned rows exceed the width.
Comments remain attached to their original values or rows.

Separators support ``:`` or ``=`` and independent space-count or column positioning before the separator and value.
Passed columns use minimum safe spacing.
Continuations support a positive number of spaces or tabs.
Structural indentation changes preserve additional payload indentation.

Defaults are 118 code points per line, bracket column four, suffix fill, spaces inside section brackets and around dots,
a closing list star, separator column 29, one space after separators and bullets, four-space continuations, and retained
list layouts.
Default rules give relative sections bracket column six and suffix width four, and meta values lowercase names with
``name: value`` spacing.

Line width is a soft limit measured in code points, counting a tab as one.
Indivisible literals and comments may exceed it.
Blank lines, ordering, newline conventions and final-newline presence are preserved.
Added lines use the current statement's newline convention, falling back to the document's first newline or LF.
Changed signed source produces a warning; signing remains a separate operation.
Default formatting is idempotent.

Interface
=========

.. doxygenenum:: erbsland::conf::syntax::BooleanForm
.. doxygenstruct:: erbsland::conf::syntax::Diagnostic
    :members:
.. doxygenclass:: erbsland::conf::syntax::Document
    :members:

.. doxygentypedef:: erbsland::conf::syntax::DocumentPtr
.. doxygenclass:: erbsland::conf::syntax::Format
    :members:
.. doxygenclass:: erbsland::conf::syntax::FormatResult
    :members:
.. doxygenstruct:: erbsland::conf::syntax::FormatRule
    :members:
.. doxygenclass:: erbsland::conf::syntax::FormatStyle
    :members:
.. doxygenclass:: erbsland::conf::syntax::FormatStylePatch
    :members:
.. doxygenclass:: erbsland::conf::syntax::Formatter
    :members:
.. doxygenenum:: erbsland::conf::syntax::IndentationStyle
.. doxygenenum:: erbsland::conf::syntax::MatrixStyle
.. doxygenclass:: erbsland::conf::syntax::Parser
    :members:
.. doxygenenum:: erbsland::conf::syntax::PositionMode
.. doxygenclass:: erbsland::conf::syntax::Selector
    :members:
.. doxygenclass:: erbsland::conf::syntax::Statement
    :members:
.. doxygenclass:: erbsland::conf::syntax::Token
    :members:

.. doxygentypedef:: erbsland::conf::syntax::TokenPtr
.. doxygenclass:: erbsland::conf::syntax::TokenKind
    :members:
.. doxygenenum:: erbsland::conf::syntax::ValueWrapping
.. doxygenenum:: erbsland::conf::syntax::VectorStyle
.. doxygenenum:: erbsland::conf::syntax::WordCase
.. doxygenenum:: erbsland::conf::syntax::WordSeparator
