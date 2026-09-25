..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Compatibility Format Specifications
    single: C++ Format Compatibility
    single: Compact Format Specification
    single: Escaped Text Format Specification

***********************************
Compatibility Format Specifications
***********************************

Existing C++ formatting knowledge is useful when reading compact patterns such as ``{:08x}`` or ``{:.2f}``.
Erbsland Core supports a practical subset of that syntax so small patterns and migrated code remain familiar.

This compatibility layer is deliberately narrower than ``std::format`` and less self-describing than the named Core
syntax.
Use :doc:`/topics/text_formatting/format_specifications` for new application-owned patterns, especially when a field
should enforce its value type; use this page when maintaining or translating compact specifications.

Read the Compact Grammar from Left to Right
===========================================

The supported grammar is:

.. code-block:: text

    [[fill]align][sign][#][0][width][.precision][type]

Every part is optional.
Alignment is ``<`` for left, ``>`` for right, or ``^`` for centered output, and the supported fill characters are space
and ``0``.
The sign is ``+``, ``-``, or space; ``#`` requests an alternate integer form; and a leading ``0`` before the width
requests numeric zero filling.
Width and text precision count decoded code points rather than encoded bytes.

Format Integers with Familiar Presentation Letters
==================================================

Integer presentation letters are ``d`` for decimal, ``x`` /``X`` for hexadecimal, ``b`` /``B`` for binary, and ``o``
/``O`` for octal.
Uppercase letters select uppercase digits and prefixes.
Integer precision is the minimum digit count, while width applies to the complete signed and prefixed result.

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

Limit and Align Text
====================

Text may use ``s`` explicitly or omit the presentation type.
Precision limits the source by decoded code points before alignment, which keeps a multibyte UTF-8 character intact.
Text defaults to left alignment; numeric values default to right alignment.

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

Select Floating-Point Notation
==============================

Floating-point presentation letters are ``f`` /``F`` for fixed, ``e`` /``E`` for scientific, ``g`` /``G`` for general,
and ``a`` /``A`` for hexadecimal notation.
Precision is forwarded to ``FloatFormat`` and therefore follows the selected notation.

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

Recognize the Compact Escaping Extension
========================================

The slash presentations are Erbsland Core extensions to the compatibility grammar.
``/html``, ``/json``, ``/xml``, ``/cpp``, and ``/pcre`` escape text for the selected destination.
The suffix ``-``, ``=``, ``+``, or ``*`` selects minimal, balanced, non-ASCII, or complete escaping respectively.
Precision is applied before escaping and width afterward.

Named text specifications express the same intent more clearly, for example
``{:text:escape=json,escape-amount=non-ascii}``.
The compact forms remain useful when preserving an existing pattern.

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

Know Where Compatibility Stops
==============================

Dynamic width and precision, nested replacement fields, locale-specific formatting, chrono formatting, range formatting,
and arbitrary fill characters are not part of this layer.
Integer precision is a minimum digit count, and all string widths use decoded code-point measurements for text layout.

These limits keep the implementation predictable, but they also make the named syntax the better choice when a pattern
needs to communicate more than one or two compact options.
