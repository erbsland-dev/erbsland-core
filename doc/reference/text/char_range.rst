.. index::
    single: Char Range

**********
Char Range
**********

Introduction
============

Char
----

:cpp:class:`Char <erbsland::text::Char>` stores one Unicode code point as a small value type.
Use it when you want to talk about a decoded character in Erbsland Core text APIs without mixing it up with a raw
``char`` byte, a UTF-8 code unit, or a full grapheme cluster.

The type intentionally stays simple.
It validates code point ranges, provides a few well-known values, and is cheap to copy.
It does not model language-specific text behavior such as combining marks, collation, or grapheme boundaries.

Unicode Light Database
~~~~~~~~~~~~~~~~~~~~~~

Some :cpp:class:`Char <erbsland::text::Char>` APIs use the Unicode Light database.
These are the category and case-mapping helpers such as
:cpp:func:`category() <erbsland::text::Char::category>`,
:cpp:func:`categoryGroup() <erbsland::text::Char::categoryGroup>`,
:cpp:func:`caseFolded() <erbsland::text::Char::caseFolded>`,
:cpp:func:`toLowercase() <erbsland::text::Char::toLowercase>`, and
:cpp:func:`toUppercase() <erbsland::text::Char::toUppercase>`.

If your code never calls one of those APIs, the generated Unicode Light database does not need to be linked into the
final binary.
That keeps small ASCII-only or encoding-only use cases lean while still making Unicode metadata available when you need
it.

ASCII Fast Paths
~~~~~~~~~~~~~~~~

For common ASCII checks, use :cpp:func:`isAscii() <erbsland::text::Char::isAscii>` and the templated
:cpp:func:`isAscii() <erbsland::text::Char::isAscii>` fast paths.
They do not use the Unicode Light database and are a good fit for protocol parsing, command-line tooling, and other text
formats that intentionally stay in the ASCII subset.

.. code-block:: cpp

    using Char = el::Char;

    auto value = Char{U'F'};
    if (value.isAscii<Char::AsciiCategory::HexDigit>()) {
        // Fast ASCII-only path.
    }

Integer Digit Helpers
~~~~~~~~~~~~~~~~~~~~~

:cpp:func:`digitValue() <erbsland::text::Char::digitValue>` converts ASCII decimal and Latin letter digits into a
numeric value.
:cpp:func:`isDigitValue() <erbsland::text::Char::isDigitValue>` combines that conversion with an
:cpp:class:`IntegerBase <erbsland::text::IntegerBase>` check.
Use :cpp:func:`fromDigitValue() <erbsland::text::Char::fromDigitValue>` when emitting integer digits with a selected
:cpp:enum:`LetterCase <erbsland::text::LetterCase>`.

Unicode Categories
~~~~~~~~~~~~~~~~~~

Use :cpp:func:`category() <erbsland::text::Char::category>` when you need the exact Unicode general category and
:cpp:func:`categoryGroup() <erbsland::text::Char::categoryGroup>` when the broad class is enough.
The convenience predicates
:cpp:func:`isCategory() <erbsland::text::Char::isCategory>`,
:cpp:func:`isCategoryGroup() <erbsland::text::Char::isCategoryGroup>`, and
``is()`` help keep that intent visible in your code.

.. code-block:: cpp

    auto codePoint = el::Char{U'_'};
    if (codePoint.isCategory(el::UnicodeCategory::ConnectorPunctuation)) {
        // Handle connector punctuation explicitly.
    }

Display Width
~~~~~~~~~~~~~

``displayWidth()`` returns the approximate cell width for one Unicode code point.
It uses the Unicode Light database and returns ``0`` for Unicode control characters, invalid code points, and internal
signal values.
Use it for straightforward alignment and measuring tasks where a per-code-point width is enough.

Simple Case Mapping
~~~~~~~~~~~~~~~~~~~

:cpp:func:`caseFolded() <erbsland::text::Char::caseFolded>`,
:cpp:func:`toLowercase() <erbsland::text::Char::toLowercase>`, and
:cpp:func:`toUppercase() <erbsland::text::Char::toUppercase>` all use simple one-code-point mappings.
That makes them fast and predictable for character-wise processing.

This also means they intentionally do not perform full Unicode case mappings that can expand to multiple code points.
For example, these helpers are meant for one code point in, one code point out.

Use ``toAsciiLowercase()`` and ``toAsciiUppercase()`` for ASCII-only folding.
These helpers only map ``A-Z`` and ``a-z`` and do not use the Unicode database.

Basic Usage
~~~~~~~~~~~

Create a :cpp:class:`Char <erbsland::text::Char>` from a ``char32_t`` code point and use
:cpp:func:`toRawValue() <erbsland::text::Char::toRawValue>` when you need the underlying value again.

.. code-block:: cpp

    auto letter = erbsland::text::Char{U'A'};
    if (letter.isValidUnicode()) {
        auto codePoint = letter.toRawValue();
    }

The default value is the null character.
Use :cpp:func:`isNull() <erbsland::text::Char::isNull>` when this matters for your logic.

Signals
~~~~~~~

Some text APIs use reserved invalid :cpp:class:`Char <erbsland::text::Char>` values as non-character signals.
The reserved signal range is ``0xFFFFFF00`` through ``0xFFFFFFFF``.
Current signals are ``endOfData()`` with raw value ``0xFFFFFFFF`` and ``noCodePoint()`` with raw value ``0xFFFFFFFE``.

Use ``isSignal()``, ``isEndOfData()``, and ``isNoCodePoint()`` when a reader or character-access API can report these
states.
Signals are not valid Unicode code points, and text-output APIs must not encode them as Unicode characters.

Validation
~~~~~~~~~~

:cpp:func:`isValidUnicode() <erbsland::text::Char::isValidUnicode>` checks whether the value is a valid Unicode code
point.
It rejects values above ``U+10FFFF`` and surrogate code points.

When a decoder encounters invalid input in a tolerant API, it can return
:cpp:func:`replacement() <erbsland::text::Char::replacement>`.
You can detect that value with :cpp:func:`isReplacement() <erbsland::text::Char::isReplacement>`.

Char Range
----------

:cpp:class:`CharRange <erbsland::text::CharRange>` describes a closed range of Unicode scalar values.
It is the range building block used by :cpp:class:`CharSet <erbsland::text::CharSet>`.

The type is tolerant at construction time.
Invalid endpoints create an empty range, while two valid endpoints are ordered automatically.
This keeps call sites simple when user input may provide the bounds in either order.

Unicode Scalar Values
~~~~~~~~~~~~~~~~~~~~~

The range stores only endpoint values.
If a range spans the UTF-16 surrogate area, membership and exports still treat surrogate code points as invalid Unicode
scalar values.

.. code-block:: cpp

    auto range = el::CharRange{el::Char{U'Z'}, el::Char{U'A'}};
    if (range.contains(el::Char{U'M'})) {
        // The endpoints were normalized to A-Z.
    }

Char Set
--------

:cpp:class:`CharSet <erbsland::text::CharSet>` stores a normalized set of Unicode scalar values.
It is the reusable character-set type for APIs such as
:cpp:func:`findFirstOf() <erbsland::text::U8StringView::findFirstOf>` and
:cpp:func:`containsOneOf() <erbsland::text::U8StringView::containsOneOf>`.

The set uses copy-on-write storage, so passing and copying sets is cheap until a copy is modified.
Internally, adjacent and overlapping :cpp:class:`CharRange <erbsland::text::CharRange>` values are merged.
Duplicate characters and invalid code points are ignored.

Creating Sets
~~~~~~~~~~~~~

Create a set from individual characters, ranges, UTF-8 text, or standard-library containers.
UTF-8 input is decoded tolerantly, matching the rest of the string API: invalid byte sequences contribute the
replacement character.

.. code-block:: cpp

    using Char = el::Char;
    using CharRange = el::CharRange;
    using CharSet = el::CharSet;

    auto digits = CharSet::fromRange(Char{U'0'}, Char{U'9'});
    auto hexLetters = CharSet::fromRange(Char{U'A'}, Char{U'F'}) | CharSet::fromRange(Char{U'a'}, Char{U'f'});
    auto separators = CharSet{u8",;"_el};

Use :cpp:func:`CharSet::fromRange() <erbsland::text::CharSet::fromRange>` when two characters describe an inclusive
range.
The expression ``CharSet{Char{U'0'}, Char{U'9'}}`` creates a set containing only the two characters ``0`` and ``9``.

Use :cpp:func:`CharSet::from(AsciiCategory) <erbsland::text::CharSet::from>` for ASCII-only classification sets that do
not link the Unicode data layer.
Use :cpp:func:`CharSet::from(UnicodeCategory) <erbsland::text::CharSet::from>` for sets derived from a Unicode general
category.
Use :cpp:func:`CharSet::fromPattern() <erbsland::text::CharSet::fromPattern>` for compact literal/range patterns where
``-`` between two characters defines an inclusive range and a leading or trailing ``-`` is a literal hyphen.

.. code-block:: cpp

    auto optionName = el::CharSet::fromPattern("-a-zA-Z0-9_"_el);

Set Operations
~~~~~~~~~~~~~~

Use named methods when clarity matters, or operators when the expression reads naturally.

.. code-block:: cpp

    auto identifierStart = letters | CharSet{Char{U'_'}};
    auto identifierContinue = identifierStart.unitedWith(digits);

    if (identifierStart <= identifierContinue) {
        // Every start character is also allowed as a continuation character.
    }

Case Helpers
~~~~~~~~~~~~

:cpp:func:`toLowercase() <erbsland::text::CharSet::toLowercase>` and
:cpp:func:`toUppercase() <erbsland::text::CharSet::toUppercase>` apply simple one-code-point mappings and normalize
duplicates.

Use ``isEqualToIgnoringCase()`` and ``isSubsetOfIgnoringCase()`` for simple Unicode case-insensitive membership checks.

Exporting Characters
~~~~~~~~~~~~~~~~~~~~

Use ``toList()`` or ``toSet()`` when another API needs the individual characters.
Use ``toString()``, ``toU8String()``, ``toU16String()``, or ``toU32String()`` when the set should be materialized as
text in ascending code-point order.

Char Signal
-----------

:cpp:enum:`CharSignal <erbsland::text::CharSignal>` names reserved non-character values that can be stored in
:cpp:class:`Char <erbsland::text::Char>`.
Use these signals with reader and character-access APIs that must distinguish the end of available data from an invalid
or absent position.

``EndOfData`` marks the exact position just after the last available character.
``NoCodePoint`` marks an invalid, no-index, or out-of-range position.

Unicode Category
----------------

:cpp:enum:`UnicodeCategory <erbsland::text::UnicodeCategory>` represents one Unicode general category such as
uppercase letters, decimal numbers, or spacing separators.
You get it from :cpp:func:`Char::category() <erbsland::text::Char::category>` when you need the precise category for one
code point.

These values follow the standard Unicode general-category model.
That makes them useful for validation, tokenization, and simple character classifiers where you want stable, well-known
semantics instead of ad-hoc tables.

Unicode Category Group
----------------------

:cpp:enum:`UnicodeCategoryGroup <erbsland::text::UnicodeCategoryGroup>` is the coarse Unicode general-category group.
It lets you ask broader questions like "is this code point a letter?" or "is this code point a separator?" without
caring about the exact subcategory.

You usually reach it through :cpp:func:`Char::categoryGroup() <erbsland::text::Char::categoryGroup>`.
That keeps user code short and readable when the precise subcategory would add no extra value.

Combined Char
-------------

``CombinedChar`` stores one base Unicode code point and up to two combining marks.
It is a small text-domain value type for places that need to keep a visually combined input character together without
modeling full grapheme-cluster rules.

The type normalizes unsupported input to the Unicode replacement character.
Use ``first()`` for the leading code point, ``singleOrNull()`` for fast single-code-point checks, and ``toString()`` or
``toU32String()`` to materialize the stored sequence.

Interface
=========

.. doxygenenum:: erbsland::text::AsciiCategory
.. doxygenclass:: erbsland::text::Char
    :members:
.. doxygentypedef:: erbsland::text::CharCompareFn
.. doxygenclass:: erbsland::text::CharRange
    :members:
.. doxygenclass:: erbsland::text::CharSet
    :members:
.. doxygenenum:: erbsland::text::CharSignal
.. doxygenclass:: erbsland::text::CombinedChar
    :members:
.. doxygenenum:: erbsland::text::UnicodeCategory
.. doxygenenum:: erbsland::text::UnicodeCategoryGroup
.. doxygenfunction:: erbsland::text::ucdVersion() noexcept -> unit::Version
