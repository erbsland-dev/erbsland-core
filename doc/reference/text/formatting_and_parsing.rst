..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Formatting; Reference
    single: Text Parsing; Reference
    single: String Format Definitions
    single: String Formatter and Related Types
    single: String Parser and Reader

***************************
Text Formatting and Parsing
***************************

String Format Definitions
=========================

Introduction
------------

Boolean Format
~~~~~~~~~~~~~~

:cpp:class:`BooleanFormat <erbsland::text::BooleanFormat>` describes how boolean values are written to Erbsland
Core strings and stream ``print`` helpers.
The default format writes lowercase ``true`` and ``false``.
Use the style factories to select ``true`` /``false``, ``yes`` /``no``, ``on`` /``off``, or ``enabled`` /``disabled``
output.
Use :cpp:enum:`Capitalization <erbsland::text::Capitalization>` to switch between lowercase, uppercase, and titlecase
words.

Use a ``BooleanFormat`` object in a print argument list to change the format for following boolean values:

.. code-block:: cpp

    el::io::printLine(el::BooleanFormat::yesNo(), "available: "_el, ready);

Byte Format
~~~~~~~~~~~

``ByteFormat`` describes how byte blocks are written as hexadecimal text.
The default format is compact lowercase output without separators, suitable for hashes and other identifiers.

Use the convenience factories for readable call sites:

.. code-block:: cpp

    auto hash = el::String::fromByteBlock(data);
    auto bytes = el::String::fromByteBlock(data, el::ByteFormat::separated());
    auto dump = el::String::fromByteBlock(data, el::ByteFormat::memoryDump());

Use a ``ByteFormat`` object in a print argument list to change the format for following byte blocks:

.. code-block:: cpp

    el::io::printLine("hash: "_el, data);
    el::io::print(el::ByteFormat::memoryDump(), data);

Bounded Output and Truncation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use ``setMaximum()`` to limit the number of byte-like output items.
The default is ``ByteLength::infinite()``.
If the byte block exceeds this maximum, ``setTruncateMode()`` selects which source bytes are retained and
``setEllipsis()`` provides the inserted truncation marker.

A non-empty ellipsis occupies one output item, just like one formatted byte.
Therefore, a maximum of 16 emits at most 15 source bytes and one ellipsis.
A maximum of zero always emits nothing, while a maximum of one emits only a non-empty ellipsis when truncation is
required.
An empty ellipsis does not occupy an item.
Middle truncation assigns an odd extra retained byte to the prefix.

The ellipsis participates in separator, byte-group, and line layout as one byte would.
Source offsets still count only actual source bytes.
Beginning and middle truncation are available only for single-line formats whose flags contain no bits other than
``Separator``.
Other layouts use end truncation.

``forDiagnostic()`` creates a compact lowercase format with a maximum of 16 items, middle truncation, and the ASCII
ellipsis ``...``:

.. code-block:: cpp

    auto diagnostic = el::String::fromByteBlock(data, el::ByteFormat::forDiagnostic());

Byte Format Flag
~~~~~~~~~~~~~~~~

``ByteFormatFlag`` controls optional byte formatting behavior.
Use ``ByteFormatFlags`` when several flags are combined.

``Separator`` inserts the configured separator between bytes.
``ByteGroups`` changes separator placement so multiple bytes are grouped together.
``Lines`` wraps output after the configured number of bytes.
``Offset`` emits a hexadecimal byte offset in front of each line.
``LineGroups`` inserts an empty line after each configured group of lines.

Float Format
~~~~~~~~~~~~

:cpp:class:`FloatFormat <erbsland::text::FloatFormat>` describes how floating-point values are written to Erbsland
Core strings and stream ``print`` helpers.
It wraps the simple presentation styles provided by ``std::format`` and supports an optional precision.

Use a :cpp:class:`FloatFormat <erbsland::text::FloatFormat>` object in a print argument list to change the format for
following floating-point values:

.. code-block:: cpp

    auto fixed = el::FloatFormat::fixed().setPrecision(el::ItemCount{2U});
    el::io::printLine("height: "_el, fixed, 18.756, " m"_el);

Float Parse Flag
~~~~~~~~~~~~~~~~

:cpp:enum:`FloatParseFlag <erbsland::text::FloatParseFlag>` controls optional floating-point parsing behavior.
Use :cpp:type:`FloatParseFlags <erbsland::text::FloatParseFlags>` when several flags are combined.

``IgnoreTrailingChars`` makes parsing stop successfully after the floating-point value.
Without this flag, parsing requires full consumption.

Float Parse Options
~~~~~~~~~~~~~~~~~~~

:cpp:class:`FloatParseOptions <erbsland::text::FloatParseOptions>` controls floating-point parsing from Erbsland
Core strings.
By default the parser accepts general floating-point syntax and requires the complete input to be consumed.

Use a style when accepted syntax must be constrained:

.. code-block:: cpp

    auto options = el::FloatParseOptions{};
    options.setStyle(el::FloatParseOptions::Style::Scientific);

Integer Base
~~~~~~~~~~~~

:cpp:class:`IntegerBase <erbsland::text::IntegerBase>` selects the syntax base for integer text conversion.
Decimal uses base 10, hexadecimal uses base 16, binary uses base 2, and octal uses base 8.

The type is an enum-like class instead of a plain enum so base-specific helpers stay with the base vocabulary.
Use :cpp:func:`baseFactor() <erbsland::text::IntegerBase::baseFactor>` for arithmetic conversion and
:cpp:func:`digitGroupSize() <erbsland::text::IntegerBase::digitGroupSize>` for separator grouping.
:cpp:func:`prefixChar() <erbsland::text::IntegerBase::prefixChar>` returns the base prefix character used after the
leading ``0`` for prefixed output.
:cpp:func:`toString() <erbsland::text::IntegerBase::toString>` returns the canonical name: ``decimal``,
``hexadecimal``, ``binary``, or ``octal``.

When parsing with :cpp:class:`IntegerParseOptions <erbsland::text::IntegerParseOptions>` and no fixed base, the parser
detects ``0x`` /``0X`` as hexadecimal, ``0b`` /``0B`` as binary, and ``0o`` /``0O`` as octal.
Otherwise it uses decimal.
Use :cpp:func:`fromPrefixChar() <erbsland::text::IntegerBase::fromPrefixChar>` when implementing matching prefix
detection logic.

Integer Format
~~~~~~~~~~~~~~

:cpp:class:`IntegerFormat <erbsland::text::IntegerFormat>` describes how integers are written to Erbsland Core strings and
:cpp:class:`AnyStringBuilder <erbsland::text::AnyStringBuilder>`.
The default format is decimal with no flags, lowercase letters, a zero field width, no precision, and negative-only sign
output.

Use the convenience factories for readable call sites:

.. code-block:: cpp

    auto decimal = el::IntegerFormat::decimal();
    auto hex = el::IntegerFormat::hexadecimal().setFlags(el::IntegerFormatFlag::BasePrefix);
    auto binary = el::IntegerFormat::binary().setFlags(el::IntegerFormatFlag::Separator);
    auto octal = el::IntegerFormat{el::IntegerBase::Octal};

Integer Format Flag
~~~~~~~~~~~~~~~~~~~

:cpp:enum:`IntegerFormatFlag <erbsland::text::IntegerFormatFlag>` controls optional integer formatting behavior.
Use :cpp:type:`IntegerFormatFlags <erbsland::text::IntegerFormatFlags>` when several flags are combined.

``ZeroFill`` pads the digit field with zeroes.
``Separator`` inserts ASCII apostrophe digit group separators.
``BasePrefix`` emits ``0x`` /``0X`` for hexadecimal, ``0b`` /``0B`` for binary, and ``0o`` /``0O`` for octal.

Integer Sign Mode
~~~~~~~~~~~~~~~~~

``IntegerSignMode`` controls whether positive values are written without a sign, with ``+``, or with a leading space.

Integer Parse Flag
~~~~~~~~~~~~~~~~~~

:cpp:enum:`IntegerParseFlag <erbsland::text::IntegerParseFlag>` controls optional integer parsing behavior.
Use :cpp:type:`IntegerParseFlags <erbsland::text::IntegerParseFlags>` when several flags are combined.

``AllowSeparator`` accepts the configured digit separator between digits.
``IgnoreTrailingChars`` makes parsing stop successfully after the integer value.
Without this flag, parsing requires full consumption.
``AcceptMinusSign`` accepts a leading ``-`` and records a negative result.
``IgnorePlusSign`` accepts a leading ``+`` without changing the result sign.
``StopAtMaximum`` stops low-level reader parsing when the configured maximum digit count is reached.

Integer Parse Options
~~~~~~~~~~~~~~~~~~~~~

:cpp:class:`IntegerParseOptions <erbsland::text::IntegerParseOptions>` controls integer parsing from Erbsland Core strings and
from :cpp:class:`StringCharReader <erbsland::text::StringCharReader>`.
``parserDefault()`` is the default option set for low-level parsers and tokenizers.
It does not accept signs or separators and reads a single integer token.
``stringDefault()`` is the default used by string-to-integer conversion.
It accepts ``-`` and ``+`` signs and requires the complete input unless ``IgnoreTrailingChars`` is set.

Use a fixed base when the syntax base is known.
A fixed base rejects base prefixes such as ``0x``, ``0b``, and ``0o``.
Leave the base unset to auto-detect these prefixes.
Use ``minimumDigits`` and ``maximumDigits`` to control the accepted digit count.
Without ``StopAtMaximum``, extra digits beyond a finite maximum are an error.
With ``StopAtMaximum``, reader parsing stops when the maximum is reached.
Use ``fixedDecimal()`` and ``fixedHex()`` for fixed-width grammar fields.
When ``AllowSeparator`` is enabled, the configured separator must appear between digits and must not start, end, or
repeat in the consumed integer.

Letter Case
~~~~~~~~~~~

:cpp:enum:`LetterCase <erbsland::text::LetterCase>` selects the letter case for text output and parsing.
It is used by integer formatting, character case-mapping, and other text operations that need case control.

Capitalization
~~~~~~~~~~~~~~

:cpp:enum:`Capitalization <erbsland::text::Capitalization>` selects the capitalization for generated words.
It is used by boolean formatting where titlecase output is useful for user-facing text.

Safe String Flag
~~~~~~~~~~~~~~~~

:cpp:enum:`SafeStringFlag <erbsland::text::SafeStringFlag>` controls optional safety checks for string operations.
Use :cpp:type:`SafeStringFlags <erbsland::text::SafeStringFlags>` when several flags are combined.

Truncate Mode
~~~~~~~~~~~~~

:cpp:enum:`TruncateMode <erbsland::text::TruncateMode>` controls how truncation is indicated when text exceeds a
configured width.

Literals
~~~~~~~~

The literal helpers in ``erbsland::text::literals`` are the preferred way to write static UTF-8 text in Erbsland Core
code.
They keep the type of the literal visible, avoid unsafe pointer-and-size pairs in user code, and let read-only APIs
refer directly to the original literal storage.

.. code-block:: cpp

    using namespace erbsland::text::literals;

    constexpr auto label = u8"Status"_el;       // U8StringLiteral<char8_t>
    auto labelText = el::U8String{u8"Status"_el}; // U8String
    auto labelEditor = el::U8StringEditor{u8"Status"_el};
    labelEditor.append(u8": ready"_el);           // explicit local mutable construction

Use ``"_el"`` when you want a constexpr-capable :cpp:class:`U8StringLiteral <erbsland::text::U8StringLiteral>`.
Use ``"_el"`` for APIs that inspect text through :cpp:class:`U8String <erbsland::text::U8String>`.
Construct :cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>` explicitly for local mutable construction or
in-place editing.

Display Escaping
----------------

``EscapeFormat::Display`` is intended for untrusted text in diagnostics and other user-facing output.
It preserves printable punctuation and Unicode text while converting control and format characters into readable C-style
sequences.
Together with ``EscapeAmount::Balanced``, it provides safe output without obscuring ordinary quotes, backslashes, or
path punctuation.

Log Escaping
------------

``EscapeFormat::Log`` applies the same balanced safety rule as display escaping, except that U+000A line feeds remain
real line breaks.
All other Unicode control and format characters are converted into visible escapes.
Use it for untrusted messages that may intentionally contain short multiline listings but must not contain terminal
control sequences, bidirectional format controls, or hidden line separators.

Markdown Escaping
-----------------

``EscapeFormat::Markdown`` protects ordinary CommonMark text.
At ``EscapeAmount::Required`` it prefixes escapable ASCII punctuation with a backslash.
Higher amounts encode controls, format characters, and requested non-ASCII code points as decimal numeric references.
This format does not protect CommonMark contexts in which backslash escaping is disabled, such as code spans and code
blocks.

String Formatter and Related Types
==================================

Introduction
------------

Format Argument
~~~~~~~~~~~~~~~

:cpp:class:`FormatArgument <erbsland::text::FormatArgument>` is the internal runtime payload used by the formatter
engine after direct values have been converted to supported argument types.
User code should extend formatting with
:cpp:struct:`FormatAs <erbsland::text::FormatAs>` instead of constructing this type directly.

Format As
~~~~~~~~~

The :cpp:struct:`FormatAs <erbsland::text::FormatAs>` template is the public extension point for formatting a custom
value.
Values with ``toString() const -> String`` are automatically formatted as UTF-8 text.
Specialize ``FormatAs`` only when formatting needs a different representation; it takes precedence over ``toString()``.
Built-in values, including integers, floating-point values, text, characters, and ``ByteBlock``, are handled directly.

Example
^^^^^^^

.. code-block:: cpp

    template <>
    struct erbsland::text::FormatAs<MyIndex> {
        [[nodiscard]] auto format(const MyIndex &value) const -> erbsland::text::String {
            return value.toString();
        }
    };

Format Strings
~~~~~~~~~~~~~~

Fields use ``{}`` or ``{index}`` for automatic or explicit argument selection.
The existing compact ``std::format`` -style specification remains available and unchanged.
An untyped ``{}`` formats a ``ByteBlock`` as compact lowercase hexadecimal text.
Legacy non-empty specifications do not select byte-formatting options.

Typed fields have this strict form:

.. code-block:: text

    {[index]:selector:option,option,...}

The selector and its trailing colon are mandatory.
For example, ``{:bytes:}`` selects default byte formatting, and ``{2:number:base=hexadecimal,alternate}`` selects the
third argument as a hexadecimal number.
Typed fields accept only the named options described below; compact legacy modifiers cannot follow a selector.

Selectors lock the accepted argument type:

*   ``text`` accepts UTF-8, UTF-16, and UTF-32 text plus ``Char``.
*   ``number`` accepts signed integers, unsigned integers, and floating-point values.
*   ``bool`` accepts only boolean values.
*   ``bytes`` accepts only ``ByteBlock``.

A selector/type mismatch raises ``FormatError``.

Named Option Grammar
^^^^^^^^^^^^^^^^^^^^

Named options are comma-separated and order-independent.
They contain no insignificant whitespace.
Option names and enum values are ASCII case-insensitive.
An option may appear only once, including through a mixture of its full name and alias.

Unsigned decimal values may omit ``=``, so ``width=32``, ``width32``, and ``w32`` are equivalent.
Enum values require ``=``.
Flags take no value.
``fill`` consumes exactly one direct safe Unicode code point and preserves its spelling.
All other characters in a named specification must also be safe Unicode, while identifiers themselves are ASCII.

Unknown options or values, duplicate options, missing values, incompatible combinations, leading or trailing empty
options, empty options between commas, and unsafe characters raise ``FormatError``.

Options by Domain
^^^^^^^^^^^^^^^^^

.. list-table::
    :header-rows: 1
    :widths: 16 50 34

    *   - Domain
        - Options
        - Notes
    *   - Shared layout
        - ``width`` / ``w``, ``alignment`` / ``al``, ``fill`` / ``fl``
        - Available for ``text``, ``number``, and ``bool``.
    *   - Text
        - ``maximum`` / ``max``, ``escape`` / ``esc``, ``escape-amount`` / ``ea``
        - Maximum truncates source code points before escaping; layout is applied afterward.
    *   - Number
        - ``base`` / ``bs``, ``notation`` / ``nt``, ``letter-case`` / ``lc``, ``sign`` / ``sg``,
          ``precision`` / ``pr``, ``alternate`` / ``alt``, ``zero-fill`` / ``zf``
        - ``alternate`` and ``zero-fill`` are flags. Integer-only and floating-point-only options reject the wrong
          numeric subtype.
    *   - Boolean
        - ``style`` / ``sty``, ``capitalization`` / ``cap``
        - Rendering uses ``BooleanFormat`` before applying the shared layout.
    *   - Bytes
        - ``separator`` / ``sep``, ``maximum`` / ``max``, ``truncate`` / ``tr``
        - ``separator`` is a flag. Truncated output uses the fixed Unicode ellipsis ``…``.

The short option aliases are globally stable and unique.
Value aliases are local to their option:

.. list-table::
    :header-rows: 1
    :widths: 25 75

    *   - Option
        - Values
    *   - ``alignment``
        - ``left`` / ``l``, ``right`` / ``r``, ``center`` / ``c``
    *   - ``escape``
        - ``none`` / ``n``, ``html`` / ``h``, ``json`` / ``j``, ``cpp`` / ``cp``, ``xml`` / ``x``,
          ``regex`` / ``rx``, ``display`` / ``d``, ``config`` / ``cf``, ``config_test`` / ``ct``
    *   - ``escape-amount``
        - ``nothing`` / ``n``, ``required`` / ``r``, ``balanced`` / ``b``, ``non-ascii`` / ``na``, ``all`` / ``a``
    *   - ``base``
        - ``decimal`` / ``d``, ``hexadecimal`` / ``x``, ``binary`` / ``b``, ``octal`` / ``o``
    *   - ``notation``
        - ``default`` / ``d``, ``fixed`` / ``f``, ``scientific`` / ``s``, ``general`` / ``g``,
          ``hexadecimal`` / ``x``
    *   - ``letter-case``
        - ``lowercase`` / ``l``, ``uppercase`` / ``u``
    *   - ``sign``
        - ``negative-only`` / ``n``, ``always`` / ``a``, ``space`` / ``s``
    *   - ``style``
        - ``true`` / ``t``, ``yes`` / ``y``, ``on`` / ``o``, ``enabled`` / ``e``
    *   - ``capitalization``
        - ``lowercase`` / ``l``, ``uppercase`` / ``u``, ``titlecase`` / ``t``
    *   - ``truncate``
        - ``end`` / ``e``, ``middle`` / ``m``, ``begin`` / ``b``

The ``bytes`` maximum has the same output-item semantics as ``ByteFormat``.
Its non-empty Unicode ellipsis occupies one item, so a maximum of 16 retains 15 bytes when truncation is necessary.
Middle truncation assigns an odd extra retained byte to the prefix.

Examples
^^^^^^^^

.. code-block:: cpp

    auto title = "{:text:maximum=20,width=24,alignment=center}"_ef.format(name);
    auto mask = "{:number:bs=x,lc=u,alternate,zf,w=10}"_ef.format(value);
    auto state = "{:bool:sty=yes,cap=titlecase}"_ef.format(enabled);
    auto diagnostic = "{:bytes:maximum=16,truncate=middle}"_ef.format(data);

String Parser and Reader
========================

Introduction
------------

String Char Reader
~~~~~~~~~~~~~~~~~~

:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` is a sequential reader for decoded Unicode code points.
It accepts :cpp:class:`U8String <erbsland::text::U8String>`,
:cpp:class:`U16String <erbsland::text::U16String>`, and
:cpp:class:`U32String <erbsland::text::U32String>` and exposes the same read API for all encodings.
Editor values are accepted for compatibility and converted to the corresponding owning read-only string.

The reader keeps the source storage alive through the read-only string object stored in its backend.
Copying a reader shares the immutable source data, but the cursor state is copied, so moving one reader forward does not
move another copy.

Positions and States
^^^^^^^^^^^^^^^^^^^^

:cpp:func:`position() <erbsland::text::StringCharReader::position>` returns the current decoded code-point index as
``unit::CpIndex``.
This is the value user code should use for diagnostics and parse errors.
``save()`` and ``restore()`` capture and restore the backend cursor and decoded position in constant time.
A saved state must be restored only to the reader that created it or to a compatible copy over the same visible text and
encoding; passing it to another reader is undefined.

Reader Operations
^^^^^^^^^^^^^^^^^

:cpp:func:`read() <erbsland::text::StringCharReader::read>` consumes the next decoded character, while
:cpp:func:`peek() <erbsland::text::StringCharReader::peek>` only inspects it.
:cpp:func:`advance() <erbsland::text::StringCharReader::advance>` skips decoded characters when the value is not
needed.

Use :cpp:func:`readIf() <erbsland::text::StringCharReader::readIf>` and
:cpp:func:`advanceIf() <erbsland::text::StringCharReader::advanceIf>` for optional grammar characters.
They leave the cursor unchanged when the next character does not match.
The string overload of ``advanceIf()`` matches a complete UTF-8 token by decoded character and restores the starting
position after a partial match or insufficient input.
Its optional character-comparison function supports cases such as ASCII-insensitive grammar tokens without converting
the source text.
The ``OrThrow`` variants reject end-of-data and malformed encoding before matching.

``readWhile()`` and ``readUntil()`` invoke a callback and return ``LoopResult`` because the callback can stop or report
an error.
``advanceWhile()`` and ``advanceUntil()`` have no callback and instead return the ``unit::CpLength`` actually skipped.
They leave the boundary character unread and stop at the configured maximum or end-of-data.
The count can be ignored when only the skip operation matters, for example when discarding optional whitespace.

Each while/until operation accepts either a :cpp:class:`CharSet <erbsland::text::CharSet>` or an
:cpp:enum:`AsciiCategory <erbsland::text::AsciiCategory>`. Prefer the category overload for standard ASCII grammar
classes such as whitespace, digits, words, URL schemes, Base64 text, and HTTP tokens.
The UTF-8, UTF-16, and UTF-32 backends decode each character once and classify it directly, without constructing a
temporary ``CharSet``.

.. code-block:: cpp

    auto reader = el::StringCharReader{source};
    reader.advanceWhile(el::AsciiCategory::Whitespace);
    reader.startCapture();
    reader.advanceWhile(el::AsciiCategory::WordWithHyphen);
    auto identifier = reader.takeCapture().toString();

Use a ``CharSet`` when the grammar has a custom or dynamic character combination that no category represents.

``parseInteger()`` parses a low-level integer token with
:cpp:class:`IntegerParseOptions <erbsland::text::IntegerParseOptions>`.
It restores the original reader position on failure and returns the parsed magnitude, sign state, resolved base, digit
count, and status.
Use ``readIntegerOrThrow<T>()`` when a reader should consume an integer token, convert it into a native or saturating
integer type, and report failures as ``ParseNumberError``.

Capture and Buffer
^^^^^^^^^^^^^^^^^^

The capture API marks a source range and returns it as an inexpensive owning read-only string slice.
Use
:cpp:func:`startCapture() <erbsland::text::StringCharReader::startCapture>` and
:cpp:func:`takeCapture() <erbsland::text::StringCharReader::takeCapture>` when the parsed token can be represented as
an unchanged slice of the input.

The reader buffer is owned parser text in the same encoding as the reader backend.
Use it for tokens that are assembled, normalized, escaped, or mixed from source text and manually appended characters.
``readToBuffer()`` and the ``readToBufferIf()`` variants consume and append one character.
``readToBufferWhile()`` and ``readToBufferUntil()`` append only accepted characters; stop, mismatch, end-of-data, and
limit characters are left unread and are not appended.

Saved reader states restore only the cursor position.
Capture and buffer state intentionally remain unchanged.

Interface
=========

.. doxygenclass:: erbsland::text::BooleanFormat
    :members:
.. doxygenclass:: erbsland::text::ByteFormat
    :members:
.. doxygenenum:: erbsland::text::ByteFormatFlag

.. doxygentypedef:: erbsland::text::ByteFormatFlags
.. doxygenenum:: erbsland::text::Capitalization
.. doxygenclass:: erbsland::text::EscapeAmount
    :members:
.. doxygenclass:: erbsland::text::EscapeFormat
    :members:
.. doxygenclass:: erbsland::text::FloatFormat
    :members:
.. doxygenenum:: erbsland::text::FloatParseFlag

.. doxygentypedef:: erbsland::text::FloatParseFlags
.. doxygenclass:: erbsland::text::FloatParseOptions
    :members:
.. doxygenclass:: erbsland::text::FormatArgument
    :members:
.. doxygenenum:: erbsland::text::FormatArgumentKind
.. doxygenstruct:: erbsland::text::FormatAs
    :members:
.. doxygenclass:: erbsland::text::FormatError
    :members:
.. doxygenclass:: erbsland::text::IntegerBase
    :members:
.. doxygenclass:: erbsland::text::IntegerFormat
    :members:
.. doxygenenum:: erbsland::text::IntegerFormatFlag

.. doxygentypedef:: erbsland::text::IntegerFormatFlags
.. doxygenenum:: erbsland::text::IntegerParseFlag

.. doxygentypedef:: erbsland::text::IntegerParseFlags
.. doxygenclass:: erbsland::text::IntegerParseOptions
    :members:
.. doxygenenum:: erbsland::text::IntegerSignMode
.. doxygenenum:: erbsland::text::LetterCase
.. doxygenclass:: erbsland::text::ParseNumberError
    :members:
.. doxygenstruct:: erbsland::text::ReadIntegerResult
    :members:
.. doxygenenum:: erbsland::text::ReadNumberStatus
.. doxygenenum:: erbsland::text::SafeStringFlag

.. doxygentypedef:: erbsland::text::SafeStringFlags
.. doxygenclass:: erbsland::text::StringCharReader
    :members:
.. doxygenclass:: erbsland::text::StringCharReaderState
    :members:
.. doxygentypedef:: erbsland::text::StringFormat
.. doxygenenum:: erbsland::text::StringSide
.. doxygenenum:: erbsland::text::TruncateMode
.. doxygenclass:: erbsland::text::U16Format
    :members:
.. doxygenclass:: erbsland::text::U32Format
    :members:
.. doxygenclass:: erbsland::text::U8Format
    :members:
