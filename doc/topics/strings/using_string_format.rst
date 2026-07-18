.. index::
    !single: String Format
    single: StringFormat
    single: U8Format
    single: U16Format
    single: U32Format
    single: FormatError
    single: AnyStringBuilder
    single: IntegerFormat
    single: FloatFormat
    single: StringEditor
    single: String
    single: StringLiteral
    single: Char
    single: Pattern Format
    single: Placeholders
    single: Format Specifications
    single: Escaped Text
    single: Supported Argument Types
    single: Positional Placeholders
    single: Integer Format Specifications
    single: Text Specifications
    single: Floating-Point Specification
    single: Format Specification Grammar

************************
How to Use String Format
************************

:cpp:type:`StringFormat <erbsland::text::StringFormat>` provides reusable
formatting patterns for generating text.

A formatting pattern combines regular text with placeholders enclosed in curly braces (``{}``).
When the pattern is applied, each placeholder is replaced with a formatted value.

The library provides a formatter type for each string encoding:

- :cpp:class:`U8Format <erbsland::text::U8Format>`
- :cpp:class:`U16Format <erbsland::text::U16Format>`
- :cpp:class:`U32Format <erbsland::text::U32Format>`

For most applications, use
:cpp:type:`StringFormat <erbsland::text::StringFormat>`, which is an alias
for :cpp:class:`U8Format <erbsland::text::U8Format>`.

If you already know Python format strings or ``std::format``, the syntax will feel familiar.
The placeholder syntax intentionally follows the same basic concepts.

Unlike ``std::format``,
:cpp:type:`StringFormat <erbsland::text::StringFormat>` parses the pattern
once and stores an internal representation.
You can then reuse the same format object many times without reparsing the pattern.

This design is especially useful when you:

- store patterns in containers,
- pass formatting patterns between functions,
- allow users to define patterns at runtime,
- validate patterns separately from formatting operations,
- or repeatedly apply the same pattern in performance-sensitive code.

Because patterns are parsed at runtime, syntax errors can be detected and reported dynamically.

Basic Usage
===========

Using a format pattern consists of two separate phases:

1. Create a
   :cpp:type:`StringFormat <erbsland::text::StringFormat>` from a pattern.
2. Apply the pattern using ``build()`` or ``appendTo()``.

When the pattern is created, the formatter validates its syntax.
If the pattern is invalid,
:cpp:class:`FormatError <erbsland::text::FormatError>` is thrown.

After a pattern has been constructed successfully, you can:

- call ``build(...)`` to create a new formatted string,
- call ``appendTo(...)`` to append formatted text to an existing
  :cpp:class:`AnyStringBuilder <erbsland::text::AnyStringBuilder>`.

Formatting also validates the supplied arguments.
A :cpp:class:`FormatError <erbsland::text::FormatError>` is thrown if the arguments do not satisfy the requirements of
the pattern.

Typical causes include:

- too few arguments,
- too many arguments,
- using an invalid argument index,
- or using a format specification that is not supported by the supplied
  argument type.

.. erbsland-demo::
    :source: text/StringFormat/FormattingPatterns.cpp
    :exec: text/string_format --demo FormattingPatterns
    :source-sha256: 41fd0bcda8d16ff5d7efde31ea105164d94110d74ae9f8159313e96809bf9715

.. code-block:: cpp

    /// `StringFormat` stores a reusable formatting pattern.
    /// The pattern uses the same placeholder syntax as `std::format`.
    ///
    /// Use `build()` to create a new string from formatted values.
    /// Use `appendTo()` to add formatted text to an existing `AnyStringBuilder` without
    /// creating temporary strings.
    void formattingPatterns() {
        // Create a reusable pattern for ISO 8601 date-time values.
        const auto isoDateTime = el::StringFormat{"{:04}-{:02}-{:02}T{:02}:{:02}:{:02}"_el};

        auto timestamp = isoDateTime.build(2026, 5, 30, 21, 41, 56);
        el::io::printLine("ISO date-time: "_el, timestamp);

        // Create a pattern to for simple HTML tags.
        const auto htmlTag = el::StringFormat{"<{0}>{1:/html}</{0}>\n"_el};

        el::AnyStringBuilder htmlOutput;
        htmlTag.appendTo(htmlOutput, "h1"_el, "Hello World"_el);
        htmlTag.appendTo(htmlOutput, "p"_el, "This paragraph was appended to a string builder."_el);
        htmlTag.appendTo(htmlOutput, "p"_el, "We add another <p> tag with \"useful\" text."_el);

        el::io::printLine("HTML output:"_el);
        el::io::print(htmlOutput);
    }

.. erbsland-ansi::
    :escape-char: ␛

    ISO date-time: 2026-05-30T21:41:56
    HTML output:
    <h1>Hello World</h1>
    <p>This paragraph was appended to a string builder.</p>
    <p>We add another &lt;p&gt; tag with &quot;useful&quot; text.</p>

.. erbsland-demo-end::

The Pattern Format
==================

A pattern consists of regular Unicode text and placeholders enclosed in curly braces (``{}``).

Literal braces must be escaped by doubling them:

- ``{{`` inserts ``{``
- ``}}`` inserts ``}``

Placeholders
------------

A placeholder selects an argument and optionally specifies how that argument should be formatted.

The simplest placeholder is ``{}``.

.. erbsland-demo::
    :source: text/StringFormat/EmptyPlaceholder.cpp
    :exec: text/string_format --demo EmptyPlaceholder
    :source-sha256: 97da395741c84458a05993f32a3a5146d412165d3937b2788f22c191b1d2fb7f

.. code-block:: cpp

    /// An empty placeholder accepts any supported type and formats it using the default format.
    void emptyPlaceholder() {
        const auto pattern = el::StringFormat("a: {} b: {} c: {} d: {}");
        el::io::printLine(pattern.build(123, false, 76.92, "text"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    a: 123 b: false c: 76.92 d: text

.. erbsland-demo-end::

An empty placeholder consumes the next argument and formats it using the default formatting rules for its type.

Placeholders are processed from left to right.
Each placeholder requires a corresponding argument when the pattern is applied.

Positional Placeholders
-----------------------

A placeholder can explicitly select an argument by index.

For example, ``{1}`` refers to the second argument supplied to the format operation.

.. erbsland-demo::
    :source: text/StringFormat/PositionalPlaceholders.cpp
    :exec: text/string_format --demo PositionalPlaceholders
    :source-sha256: 40cbe6bcb13b3eb5a0bf3e83a96e949ddb1db513030bf28e7e2783a1c63d27a1

.. code-block:: cpp

    /// Positional placeholders allow you to specify the order of the arguments in the format string.
    void positionalPlaceholders() {
        const auto pattern = el::StringFormat("<{1}>{0}</{1}>");
        el::io::printLine(pattern.build("text"_el, "h1"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    <h1>text</h1>

.. erbsland-demo-end::

You may reference the same argument multiple times.

Positional placeholders must form a consecutive sequence of indexes starting at zero.

.. code-block:: cpp

    el::StringFormat("{1},{2}"_el) // ERROR: index 0 is missing.

Format Specifications
---------------------

A placeholder may contain a format specification after a colon (``:``).

.. code-block:: text

    {:<format>}
    {<position>:<format>}

If no argument position is specified, the placeholder consumes the next positional argument.

The format specification controls how the selected value is converted to text.
The supported options depend on the type being formatted.

Format Specification Grammar
----------------------------

The formatter implements a compact and predictable subset inspired by ``std::format``.

.. code-block:: text

    [[fill]align][sign][#][0][width][.precision][type]

All elements are optional.

``align`` may be:

- ``<`` left alignment
- ``>`` right alignment
- ``^`` centered alignment

Only space and ``0`` are supported as fill characters.

Examples:

.. code-block:: text

    {:0>8}
    {: >8}

``sign`` may be:

- ``+`` always display a sign
- ``-`` display a sign only for negative values
- space display a leading space for positive values

``#`` requests a base prefix for non-decimal integer formats.

``width`` and text precision are measured in decoded Unicode code points, not bytes.

Differences From std::format And Python
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This formatting system intentionally supports only the subset required by Erbsland Core.

Notable differences include:

- Only space and ``0`` are supported as fill characters.
- Dynamic width and precision are not supported.
- Nested replacement fields are not supported.
- Locale-specific formatting is not supported.
- Chrono and range formatting are not supported.
- ``/html``, ``/json``, ``/xml``, ``/cpp``, and ``/pcre`` are Erbsland
  extensions.
- Integer precision specifies a minimum digit count.
- Escaped-text precision is applied before escaping.
- Width and text precision use decoded code-point counts rather than byte
  counts.

Integer Format Specifications
-----------------------------

Integer arguments support the following presentation types:

- ``d`` — decimal (base 10), the default
- ``x`` / ``X`` — hexadecimal (base 16)
- ``b`` / ``B`` — binary (base 2)
- ``o`` / ``O`` — octal (base 8)

A leading zero before the width enables zero-padding.

Integer precision specifies a minimum digit count.
Additional zeroes are inserted after the sign and base prefix, if present.

.. erbsland-demo::
    :source: text/StringFormat/IntegerFormats.cpp
    :exec: text/string_format --demo IntegerFormats
    :source-sha256: 468661b78094fcb9b102f14fb220abf032ca234c8bb6ce7546f51b495fa28b50

.. code-block:: cpp

    /// Integer format specifications control the base and padding of numeric output.
    void integerFormats() {
        const auto pattern = el::StringFormat("{:+d} {:#x} {:.4d} {:8.4d}"_el);
        el::io::printLine(pattern.build(42, 42, 42, 42));
    }

.. erbsland-ansi::
    :escape-char: ␛

    +42 0x2a 0042     0042

.. erbsland-demo-end::

For advanced integer formatting features such as digit grouping and custom base-prefix handling, see
:cpp:class:`IntegerFormat <erbsland::text::IntegerFormat>`.

Text Specifications
-------------------

Text arguments support ``s`` as an explicit presentation type.

Precision limits the number of decoded code points before alignment and padding are applied.

If no alignment is specified:

- text values are left-aligned,
- numeric values are right-aligned.

.. erbsland-demo::
    :source: text/StringFormat/TextFormats.cpp
    :exec: text/string_format --demo TextFormats
    :source-sha256: 2653d5fb04acae64dfc88051a6561f4b8106ab765ccb3dd2226f19f9526d5cf4

.. code-block:: cpp

    /// Text format specifications control the alignment and truncation of text output.
    void textFormats() {
        const auto textPattern = el::StringFormat("{:<8}|{:>8}|{:^8}|{:.3s}"_el);
        el::io::printLine(textPattern.build("cat"_el, "cat"_el, "cat"_el, "abcdef"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    cat     |     cat|  cat   |abc

.. erbsland-demo-end::

Floating-Point Specifications
-----------------------------

Floating-point arguments support:

- ``f`` / ``F`` — fixed notation
- ``e`` / ``E`` — scientific notation
- ``g`` / ``G`` — general notation
- ``a`` / ``A`` — hexadecimal notation

Precision is forwarded to
:cpp:class:`FloatFormat <erbsland::text::FloatFormat>`.

.. erbsland-demo::
    :source: text/StringFormat/FloatFormats.cpp
    :exec: text/string_format --demo FloatFormats
    :source-sha256: a52b959ce58e4b99d7d57d9e14d27cb3b83f870f91ef218e107374fce96b0969

.. code-block:: cpp

    /// Float format specifications control the precision and sign of floating-point output.
    void floatFormats() {
        const auto floatPattern = el::StringFormat("{:.2f} {:+8.1f}"_el);
        el::io::printLine(floatPattern.build(12.345, 1.25));
    }

.. erbsland-ansi::
    :escape-char: ␛

    12.35     +1.2

.. erbsland-demo-end::

Escaped Text Specifications
---------------------------

The slash presentation type is an Erbsland extension for escaped text.

It can be used on any supported text value and may be combined with width, alignment, and precision.

Precision is applied before escaping.
Width and alignment are applied after escaping.

Supported escape formats:

- ``/html`` — HTML text escaping
- ``/json`` — JSON string escaping
- ``/xml`` — XML text escaping
- ``/cpp`` — C++ string literal escaping
- ``/pcre`` — PCRE-compatible escaping

.. erbsland-demo::
    :source: text/StringFormat/EscapeFormat.cpp
    :exec: text/string_format --demo EscapeFormat
    :source-sha256: 8a246a1d36f1e4041bcd66f724414d04d2880c44c649fa10278c02fbfd3eca35

.. code-block:: cpp

    /// Escape format specifications control the HTML escaping of text output.
    void escapeFormat() {
        const auto pattern = el::StringFormat("<p>{:/html}</p>\n{:>12/html}"_el);
        el::io::printLine(pattern.build("<script>alert(\"xss\")</script>"_el, "<p>"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    <p>&lt;script&gt;alert(&quot;xss&quot;)&lt;/script&gt;</p>
       &lt;p&gt;

.. erbsland-demo-end::

By default, escaped text uses the balanced escape mode.
This mode escapes control and invisible characters while keeping readable text intact.

An optional suffix modifies the escape intensity:

- ``/html-`` — minimal escaping
- ``/html=`` — balanced escaping (default)
- ``/html+`` — escape everything except visible ASCII characters
- ``/html*`` — escape all characters

.. erbsland-demo::
    :source: text/StringFormat/EscapeAmount.cpp
    :exec: text/string_format --demo EscapeAmount
    :source-sha256: c9373595f66a1a2942ce9ca2184a95e61f20981affe79030c8833aaa7d720e96

.. code-block:: cpp

    /// Escape format amount is controlled using a suffix after the specifier.
    void escapeAmount() {
        const auto pattern = el::StringFormat("/json   : {0:/json}\n/json+  : {0:/json+}\n/json*  : {0:/json*}"_el);
        el::io::printLine(pattern.build("café\n"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    /json   : café\n
    /json+  : caf\u00E9\n
    /json*  : \u0063\u0061\u0066\u00E9\n

.. erbsland-demo-end::

Escaped text can be applied to all supported string types and character arguments.

Boolean values are first converted to ``true`` or ``false`` and then escaped.

Supported Argument Types
========================

The formatter accepts the following argument types:

- Text types —
  :cpp:type:`StringEditor <erbsland::text::StringEditor>`,
  :cpp:type:`String <erbsland::text::String>`,
  :cpp:type:`StringLiteral <erbsland::text::StringLiteral>`,
  and standard library string types such as ``std::string`` and
  ``std::string_view``.
- Signed and unsigned integer types, including saturated integers.
- Floating-point values such as ``float`` and ``double``.
- Boolean values.
- :cpp:class:`Char <erbsland::text::Char>`.
- Types that provide ``toString() const -> StringEditor``.
- Types that provide ``toRawValue() const -> T`` where ``T`` is a
  supported formatting type.

Integer values may use any integer presentation type.
Text values may additionally use escaped-text formatting.
Floating-point values use the default float conversion unless a specific presentation type is selected.
Boolean values are rendered as the lowercase English words ``true`` and ``false``.
Character values are inserted as Unicode characters.

.. erbsland-demo::
    :source: text/StringFormat/SupportedTypes.cpp
    :exec: text/string_format --demo SupportedTypes
    :source-sha256: b00b0a269cbd1e690236a6866a61176a53c822f16405c33de0c5a0da6e47e060

.. code-block:: cpp

    /// The formatter accepts text, integers, floats, booleans, and characters.
    void supportedTypes() {
        const auto pattern = el::StringFormat("{} {} {} {} {}"_el);
        el::io::printLine(pattern.build(42, 3.14, true, 'A', "hello"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    42 3.14 true A hello

.. erbsland-demo-end::
