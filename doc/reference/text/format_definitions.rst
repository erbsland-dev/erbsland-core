.. index::
    single: String Format Definitions

*************************
String Format Definitions
*************************

Introduction
============

Boolean Format
--------------

:cpp:class:`BooleanFormat <erbsland::text::BooleanFormat>` describes how boolean values are written to Erbsland
strings and stream ``print`` helpers.
The default format writes lowercase ``true`` and ``false``.
Use the style factories to select ``true`` /``false``, ``yes`` /``no``, ``on`` /``off``, or ``enabled`` /``disabled``
output.
Use :cpp:enum:`Capitalization <erbsland::text::Capitalization>` to switch between lowercase, uppercase, and titlecase
words.

Use a ``BooleanFormat`` object in a print argument list to change the format for following boolean values:

.. code-block:: cpp

    el::io::printLine(el::BooleanFormat::yesNo(), "available: "_el, ready);

Byte Format
-----------

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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
----------------

``ByteFormatFlag`` controls optional byte formatting behavior.
Use ``ByteFormatFlags`` when several flags are combined.

``Separator`` inserts the configured separator between bytes.
``ByteGroups`` changes separator placement so multiple bytes are grouped together.
``Lines`` wraps output after the configured number of bytes.
``Offset`` emits a hexadecimal byte offset in front of each line.
``LineGroups`` inserts an empty line after each configured group of lines.

Float Format
------------

:cpp:class:`FloatFormat <erbsland::text::FloatFormat>` describes how floating-point values are written to Erbsland
strings and stream ``print`` helpers.
It wraps the simple presentation styles provided by ``std::format`` and supports an optional precision.

Use a :cpp:class:`FloatFormat <erbsland::text::FloatFormat>` object in a print argument list to change the format for
following floating-point values:

.. code-block:: cpp

    auto fixed = el::FloatFormat::fixed().setPrecision(el::ItemCount{2U});
    el::io::printLine("height: "_el, fixed, 18.756, " m"_el);

Float Parse Flag
----------------

:cpp:enum:`FloatParseFlag <erbsland::text::FloatParseFlag>` controls optional floating-point parsing behavior.
Use :cpp:type:`FloatParseFlags <erbsland::text::FloatParseFlags>` when several flags are combined.

``IgnoreTrailingChars`` makes parsing stop successfully after the floating-point value.
Without this flag, parsing requires full consumption.

Float Parse Options
-------------------

:cpp:class:`FloatParseOptions <erbsland::text::FloatParseOptions>` controls floating-point parsing from Erbsland
strings.
By default the parser accepts general floating-point syntax and requires the complete input to be consumed.

Use a style when accepted syntax must be constrained:

.. code-block:: cpp

    auto options = el::FloatParseOptions{};
    options.setStyle(el::FloatParseOptions::Style::Scientific);

Integer Base
------------

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
--------------

:cpp:class:`IntegerFormat <erbsland::text::IntegerFormat>` describes how integers are written to Erbsland strings and
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
-------------------

:cpp:enum:`IntegerFormatFlag <erbsland::text::IntegerFormatFlag>` controls optional integer formatting behavior.
Use :cpp:type:`IntegerFormatFlags <erbsland::text::IntegerFormatFlags>` when several flags are combined.

``ZeroFill`` pads the digit field with zeroes.
``Separator`` inserts ASCII apostrophe digit group separators.
``BasePrefix`` emits ``0x`` /``0X`` for hexadecimal, ``0b`` /``0B`` for binary, and ``0o`` /``0O`` for octal.

Integer Sign Mode
-----------------

``IntegerSignMode`` controls whether positive values are written without a sign, with ``+``, or with a leading space.

Integer Parse Flag
------------------

:cpp:enum:`IntegerParseFlag <erbsland::text::IntegerParseFlag>` controls optional integer parsing behavior.
Use :cpp:type:`IntegerParseFlags <erbsland::text::IntegerParseFlags>` when several flags are combined.

``AllowSeparator`` accepts the configured digit separator between digits.
``IgnoreTrailingChars`` makes parsing stop successfully after the integer value.
Without this flag, parsing requires full consumption.
``AcceptMinusSign`` accepts a leading ``-`` and records a negative result.
``IgnorePlusSign`` accepts a leading ``+`` without changing the result sign.
``StopAtMaximum`` stops low-level reader parsing when the configured maximum digit count is reached.

Integer Parse Options
---------------------

:cpp:class:`IntegerParseOptions <erbsland::text::IntegerParseOptions>` controls integer parsing from Erbsland strings and
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
-----------

:cpp:enum:`LetterCase <erbsland::text::LetterCase>` selects the letter case for text output and parsing.
It is used by integer formatting, character case-mapping, and other text operations that need case control.

Capitalization
--------------

:cpp:enum:`Capitalization <erbsland::text::Capitalization>` selects the capitalization for generated words.
It is used by boolean formatting where titlecase output is useful for user-facing text.

Safe String Flag
----------------

:cpp:enum:`SafeStringFlag <erbsland::text::SafeStringFlag>` controls optional safety checks for string operations.
Use :cpp:type:`SafeStringFlags <erbsland::text::SafeStringFlags>` when several flags are combined.

Truncate Mode
-------------

:cpp:enum:`TruncateMode <erbsland::text::TruncateMode>` controls how truncation is indicated when text exceeds a
configured width.

Literals
--------

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
================

``EscapeFormat::Display`` is intended for untrusted text in diagnostics and other user-facing output.
It preserves printable punctuation and Unicode text while converting control and format characters into readable C-style
sequences.
Together with ``EscapeAmount::Balanced``, it provides safe output without obscuring ordinary quotes, backslashes, or
path punctuation.

Markdown Escaping
=================

``EscapeFormat::Markdown`` protects ordinary CommonMark text.
At ``EscapeAmount::Required`` it prefixes escapable ASCII punctuation with a backslash.
Higher amounts encode controls, format characters, and requested non-ASCII code points as decimal numeric references.
This format does not protect CommonMark contexts in which backslash escaping is disabled, such as code spans and code
blocks.

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
.. doxygenenum:: erbsland::text::SafeStringFlag

.. doxygentypedef:: erbsland::text::SafeStringFlags
.. doxygenenum:: erbsland::text::StringBomMode
.. doxygenenum:: erbsland::text::StringSide
.. doxygenenum:: erbsland::text::TruncateMode
