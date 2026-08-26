.. index::
    !single: String Format
    single: StringFormat
    single: Format Pattern
    single: Placeholder
    single: Positional Placeholder
    single: FormatError
    single: FormatAs

************************
Formatting Text Patterns
************************

Manual concatenation becomes difficult to read as soon as output has a stable shape or values appear in a different
order.
A format pattern keeps that shape in one place and lets every supplied value remain in its natural type.

:cpp:type:`StringFormat <erbsland::text::StringFormat>` stores a parsed UTF-8 pattern that can be reused.
This page explains its lifetime, placeholder selection, supported values, and the choice between building a standalone
result and appending to an existing builder.
Formatting options are intentionally separate: :doc:`format_specifications` describes the primary named syntax, while
:doc:`compatibility_format_specifications` covers the compact C++-style compatibility layer.

Create the Pattern Once
=======================

Constructing a format parses and validates its pattern.
Keeping the resulting ``StringFormat`` is useful when the same shape is applied repeatedly, because later calls do not
need to parse it again.
``U16Format`` and ``U32Format`` provide the same model when the pattern and result use another string width.

``build(...)`` returns a new string.
When a larger result is already being assembled, ``appendTo(...)`` writes directly to ``AnyStringBuilder`` and avoids a
temporary formatted string.
Supplying a wrong argument count, an invalid position, or a specification that does not accept the value type raises
``FormatError``.

.. erbsland-demo::
    :source: text/StringFormat/FormattingPatterns.cpp
    :exec: text/string_format --demo FormattingPatterns
    :source-sha256: 4abed9a193044b06fcbfcec507835bbc4551c6050deca756c7f4cd27790934dd

.. code-block:: cpp

    /// `StringFormat` stores a reusable formatting pattern.
    /// Named specifications keep the expected value type and each formatting choice visible.
    ///
    /// Use `build()` to create a new string from formatted values.
    /// Use `appendTo()` to add formatted text to an existing `AnyStringBuilder` without
    /// creating temporary strings.
    void formattingPatterns() {
        // Create a reusable pattern for ISO 8601 date-time values.
        const auto isoDateTime =
            el::StringFormat{"{:number:width=4,zero-fill}-{:number:width=2,zero-fill}-{:number:width=2,zero-fill}T"
                             "{:number:width=2,zero-fill}:{:number:width=2,zero-fill}:{:number:width=2,zero-fill}"_el};

        auto timestamp = isoDateTime.build(2026, 5, 30, 21, 41, 56);
        el::io::printLine("ISO date-time: "_el, timestamp);

        // Create a pattern for simple HTML tags.
        const auto htmlTag = el::StringFormat{"<{0}>{1:text:escape=html}</{0}>\n"_el};

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

Let Empty Placeholders Follow the Arguments
===========================================

Regular text appears unchanged in the result, while ``{}`` consumes the next argument and applies its default format.
Placeholders are processed from left to right, and every supplied argument must be consumed.
Write ``{{`` or ``}}`` when a literal brace belongs in the output.

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

Reorder and Reuse Values with Positions
=======================================

An explicit index such as ``{1}`` selects the second argument and can select the same value more than once.
Explicit positions must form a consecutive set beginning at zero, which catches missing or accidentally unused values
when the pattern is created.
Do not mix automatic and explicit selection in one pattern.

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

Pass Values Without Preparing Temporary Strings
===============================================

The formatter accepts Core and standard-library text, ``Char``, Boolean values, signed and unsigned integers,
floating-point values, saturated integers, and ``ByteBlock``.
Types with ``toString() const -> String`` participate automatically.
Specialize :cpp:struct:`FormatAs <erbsland::text::FormatAs>` when a custom type needs a different public formatting
representation.

Passing the original typed value is clearer and often more efficient than converting it into a temporary string first.
A named selector can also lock a placeholder to text, number, Boolean, or bytes so an unexpected type fails instead of
silently changing the output.

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
