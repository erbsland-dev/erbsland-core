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

Compact Format Grammar
======================

The supported grammar is:

.. code-block:: text

    [[fill]align][sign][#][0][width][.precision][type]

Every part is optional.
Alignment is ``<`` for left, ``>`` for right, or ``^`` for centered output, and the supported fill characters are space
and ``0``.
The sign is ``+``, ``-``, or space; ``#`` requests an alternate integer form; and a leading ``0`` before the width
requests numeric zero filling.
Width and text precision count decoded code points rather than encoded bytes.

Integer Presentation Letters and Padding
========================================

Integer presentation letters are ``d`` for decimal, ``x`` /``X`` for hexadecimal, ``b`` /``B`` for binary, and ``o``
/``O`` for octal.
Uppercase letters select uppercase digits and prefixes.
Integer precision is the minimum digit count, while width applies to the complete signed and prefixed result.

.. erbsland-demo::
    :source: text/StringFormat/IntegerFormats.cpp
    :exec: text/string_format --demo IntegerFormats
    :sha256: 03e7a4a126625d7e235b9b1cd993e13d76dad37a2c6814d6c9a2d30e91e405e5

.. code-block:: cpp

    /// Integer format specifications control the base and padding of numeric output.
    void integerFormats() {
        const auto pattern = el::StringFormat("{:+d} {:#x} {:.4d} {:8.4d}"_el);
        el::io::printLine(pattern.build(42, 42, 42, 42));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m+42 0x2a 0042     0042␛[0m

.. erbsland-demo-end::

Text Precision and Alignment
============================

Text may use ``s`` explicitly or omit the presentation type.
Precision limits the source by decoded code points before alignment, which keeps a multibyte UTF-8 character intact.
Text defaults to left alignment; numeric values default to right alignment.

.. erbsland-demo::
    :source: text/StringFormat/TextFormats.cpp
    :exec: text/string_format --demo TextFormats
    :sha256: 2694cf5c58d007c352019cea812efa3d45a6c8b2d9972fe50aa46b291450e382

.. code-block:: cpp

    /// Text format specifications control the alignment and truncation of text output.
    void textFormats() {
        const auto textPattern = el::StringFormat("{:<8}|{:>8}|{:^8}|{:.3s}"_el);
        el::io::printLine(textPattern.build("cat"_el, "cat"_el, "cat"_el, "abcdef"_el));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mcat     |     cat|  cat   |abc␛[0m

.. erbsland-demo-end::

Floating-Point Notation and Precision
=====================================

Floating-point presentation letters are ``f`` /``F`` for fixed, ``e`` /``E`` for scientific, ``g`` /``G`` for general,
and ``a`` /``A`` for hexadecimal notation.
Precision is forwarded to ``FloatFormat`` and therefore follows the selected notation.

.. erbsland-demo::
    :source: text/StringFormat/FloatFormats.cpp
    :exec: text/string_format --demo FloatFormats
    :sha256: 16453a2d765de3621d16a68614b92c0e356959bc5a7e8074a1fb656f443da728

.. code-block:: cpp

    /// Float format specifications control the precision and sign of floating-point output.
    void floatFormats() {
        const auto floatPattern = el::StringFormat("{:.2f} {:+8.1f}"_el);
        el::io::printLine(floatPattern.build(12.345, 1.25));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m12.35     +1.2␛[0m

.. erbsland-demo-end::

Compact Text Escaping Extensions
================================

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
    :sha256: bd83ad0b49341d0d8c79fa281545c346526ad43bfcf497aea63ea7fa4426136b

.. code-block:: cpp

    /// Escape format specifications control the HTML escaping of text output.
    void escapeFormat() {
        const auto pattern = el::StringFormat("<p>{:/html}</p>\n{:>12/html}"_el);
        el::io::printLine(pattern.build("<script>alert(\"xss\")</script>"_el, "<p>"_el));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m<p>&lt;script&gt;alert(&quot;xss&quot;)&lt;/script&gt;</p>
           &lt;p&gt;␛[0m

.. erbsland-demo-end::

.. erbsland-demo::
    :source: text/StringFormat/EscapeAmount.cpp
    :exec: text/string_format --demo EscapeAmount
    :sha256: 4538a2d65b2f6141525de856aa5b1ac60fdc8196debe3e4d3b43085e6c30eae4

.. code-block:: cpp

    /// Escape format amount is controlled using a suffix after the specifier.
    void escapeAmount() {
        const auto pattern = el::StringFormat("/json   : {0:/json}\n/json+  : {0:/json+}\n/json*  : {0:/json*}"_el);
        el::io::printLine(pattern.build("café\n"_el));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m/json   : café\n
        /json+  : caf\u00E9\n
        /json*  : \u0063\u0061\u0066\u00E9\n␛[0m

.. erbsland-demo-end::

Unsupported Compatibility Format Features
=========================================

Dynamic width and precision, nested replacement fields, locale-specific formatting, chrono formatting, range formatting,
and arbitrary fill characters are not part of this layer.
Integer precision is a minimum digit count, and all string widths use decoded code-point measurements for text layout.

These limits keep the implementation predictable, but they also make the named syntax the better choice when a pattern
needs to communicate more than one or two compact options.
