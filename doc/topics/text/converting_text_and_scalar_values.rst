..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Converting Text and Scalar Values
    single: Boolean Conversion
    single: Integer Conversion
    single: Floating-Point Conversion
    single: BooleanFormat
    single: IntegerParseOptions
    single: IntegerFormat
    single: FloatParseOptions
    single: FloatFormat

*********************************
Converting Text and Scalar Values
*********************************

Configuration values, command-line fields, and protocol attributes often arrive as text even though the application
needs a Boolean or a number.
The opposite boundary appears when those values must become stable text again for a report, identifier, or encoded
message.

Erbsland Core keeps both directions on the string types.
The ``toBoolean()``, ``toInteger()``, and ``toFloat()`` families read a complete value, while ``fromBoolean()``,
``fromInteger()``, and ``fromFloat()`` create text using explicit formatting options.
This page shows how to choose failure handling and how the parse and output options change the accepted or generated
representation.

Choose What Invalid Input Means
===============================

Each text-to-value family has a fallback-returning and a throwing form.
The fallback form is convenient when invalid or missing text has an ordinary default, such as an optional setting.
The ``OrThrow`` form is better when invalid text must be reported to the caller because it preserves the distinction
between a valid value and a fallback.

By default, conversion consumes the complete string.
It does not trim whitespace, and integer or floating-point overflow is an error rather than a wrapped result.
Trim explicitly before conversion when surrounding whitespace belongs to the input format; enable trailing input only
when the grammar deliberately places another field after the value.

Read and Write Boolean Values
=============================

Boolean input recognizes ``true``, ``on``, ``yes``, ``enabled``, ``false``, ``off``, ``no``, and ``disabled`` without
regard to ASCII letter case.
The match must cover the complete text, so padding and partial words remain invalid.

:cpp:class:`BooleanFormat <erbsland::text::BooleanFormat>` controls the reverse conversion.
Its four styles produce ``true`` /``false``, ``yes`` /``no``, ``on`` /``off``, or ``enabled`` /``disabled``.
:cpp:enum:`Capitalization <erbsland::text::Capitalization>` then selects lowercase, uppercase, or titlecase words.

.. erbsland-demo::
    :source: text/ScalarConversion/BooleanValues.cpp
    :exec: text/scalar_conversion --demo BooleanValues
    :source-sha256: 9a847fa56d69bba237e99db2de26fc42c39b2df30d4737bc125d84d8ac9d00c9

.. code-block:: cpp

    /// Convert complete Boolean literals and choose the words used for Boolean output.
    void booleanValues() {
        const auto onlineText = el::String{"YES"_el};
        const auto sleepingText = el::String{"disabled"_el};
        const auto paddedText = el::String{" yes "_el};

        el::io::printLine("YES .............: "_el, onlineText.toBooleanOrThrow());
        el::io::printLine("disabled ........: "_el, sleepingText.toBooleanOrThrow());
        el::io::printLine("padded fallback .: "_el, paddedText.toBoolean(true));

        try {
            const auto unexpectedValue = el::String{"misschien"_el}.toBooleanOrThrow();
            el::io::printLine("misschien .......: "_el, unexpectedValue);
        } catch (const el::Exception &) {
            el::io::printLine("misschien .......: parse error"_el);
        }

        auto titleYesNo = el::BooleanFormat::yesNo();
        titleYesNo.setCapitalization(el::Capitalization::Titlecase);
        auto upperEnabled = el::BooleanFormat::enabledDisabled();
        upperEnabled.setCapitalization(el::Capitalization::Uppercase);
        const auto onlineOutput = el::String{el::StringEditor::fromBoolean(true, titleYesNo)};
        const auto offlineOutput = el::String{el::StringEditor::fromBoolean(false, upperEnabled)};

        el::io::printLine("Formatted status words:"_el);
        el::io::printLine("  "_el, onlineOutput);
        el::io::printLine("  "_el, offlineOutput);
    }

.. erbsland-ansi::
    :escape-char: ␛

    YES .............: true
    disabled ........: false
    padded fallback .: true
    misschien .......: parse error
    Formatted status words:
      Yes
      DISABLED

.. erbsland-demo-end::

Read Integer Syntax Deliberately
================================

``toInteger<T>()`` and ``toIntegerOrThrow<T>()`` convert directly to the requested signed, unsigned, or saturated
integer type and check its range.
:cpp:class:`IntegerParseOptions <erbsland::text::IntegerParseOptions>` describes the grammar when the defaults do not
match the input format.

With no fixed base, the parser recognizes ``0x``, ``0b``, and ``0o`` prefixes and otherwise reads decimal text.
Setting a fixed base selects decimal, hexadecimal, binary, or octal and rejects those prefixes.
The minimum and maximum digit counts are useful for fixed-width fields, while ``fixedDecimal()`` and ``fixedHex()``
express the common exact-width cases directly.

The parse flags answer less common grammar questions:

* ``AllowSeparator`` accepts the configured digit-group separator between digits.
* ``AcceptMinusSign`` and ``IgnorePlusSign`` control leading signs.
* ``IgnoreTrailingChars`` accepts a value followed by another field.
* ``StopAtMaximum`` lets a low-level reader stop after the maximum digit count instead of treating more digits as an
  error.

The ordinary string conversion default already accepts ``-`` and ``+`` signs and requires complete input.
``parserDefault()`` is intentionally stricter because it is designed for a parser that controls the surrounding grammar.

Format Integers for Their Destination
=====================================

:cpp:class:`IntegerFormat <erbsland::text::IntegerFormat>` starts with ordinary decimal output.
The decimal, hexadecimal, binary, and octal factories make another base visible at the call site.
``BasePrefix`` adds ``0x``, ``0b``, or ``0o`` where applicable, ``Separator`` groups digits with apostrophes, and
``ZeroFill`` pads the digit field with zeroes.

Letter case affects hexadecimal digits and prefixes.
Field width controls the minimum digit field, precision controls the minimum digit count, and
:cpp:enum:`IntegerSignMode <erbsland::text::IntegerSignMode>` chooses negative-only, always, or leading-space sign
output.
The following demo places parsing and formatting together because the destination syntax should usually be chosen
independently of the source spelling.

.. erbsland-demo::
    :source: text/ScalarConversion/IntegerValues.cpp
    :exec: text/scalar_conversion --demo IntegerValues
    :source-sha256: f37e1f86da597bebcbf0cf2206a2897ef33f5f1489ff056b07601239a6094b4f

.. code-block:: cpp

    /// Parse complete integer values and format them for protocol and display use.
    void integerValues() {
        const auto nodeId = el::String{"0x2A"_el}.toIntegerOrThrow<unsigned int>();

        auto groupedOptions = el::IntegerParseOptions::stringDefault();
        groupedOptions.addFlags(el::IntegerParseFlag::AllowSeparator);
        const auto sampleCount = el::String{"12'500"_el}.toIntegerOrThrow<int>(groupedOptions);
        const auto invalidPort = el::String{"70000"_el}.toInteger<uint16_t>(uint16_t{9000U});

        el::io::printLine("Node id ...........: "_el, nodeId);
        el::io::printLine("Measurements ......: "_el, sampleCount);
        el::io::printLine("Invalid port ......: "_el, invalidPort, " (fallback)"_el);

        try {
            const auto unexpectedValue = el::String{"12x"_el}.toIntegerOrThrow<int>();
            el::io::printLine("12x ...............: "_el, unexpectedValue);
        } catch (const el::Exception &) {
            el::io::printLine("12x ...............: parse error"_el);
        }

        auto hexadecimal = el::IntegerFormat::hexadecimal();
        hexadecimal.addFlags(el::IntegerFormatFlag::BasePrefix | el::IntegerFormatFlag::ZeroFill)
            .setLetterCase(el::LetterCase::Uppercase)
            .setFieldWidth(el::CpLength{6U});

        auto groupedDecimal = el::IntegerFormat::decimal();
        groupedDecimal.addFlags(el::IntegerFormatFlag::Separator).setSignMode(el::IntegerSignMode::Always);

        el::io::printLine("Hexadecimal .......: "_el, el::String::fromInteger(nodeId, hexadecimal));
        el::io::printLine("Grouped ...........: "_el, el::String::fromInteger(sampleCount, groupedDecimal));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Node id ...........: 42
    Measurements ......: 12500
    Invalid port ......: 9000 (fallback)
    12x ...............: parse error
    Hexadecimal .......: 0X00002A
    Grouped ...........: +12'500

.. erbsland-demo-end::

Read and Write Floating-Point Values
====================================

Floating-point conversion follows the same fallback-versus-exception model.
:cpp:class:`FloatParseOptions <erbsland::text::FloatParseOptions>` accepts general, fixed, scientific, or hexadecimal
syntax.
General syntax accepts ordinary fixed or scientific notation; a narrower style is useful when the input format promises
one particular representation.
``IgnoreTrailingChars`` is the single parse flag and should only be enabled when the remaining text is intentionally
handled elsewhere.

:cpp:class:`FloatFormat <erbsland::text::FloatFormat>` selects default, fixed, scientific, general, or hexadecimal
output.
Precision has the meaning of the selected notation, and letter case controls notation letters such as ``E`` and the
hexadecimal exponent marker.

.. erbsland-demo::
    :source: text/ScalarConversion/FloatingPointValues.cpp
    :exec: text/scalar_conversion --demo FloatingPointValues
    :source-sha256: 18d337a0b8ff75f53d261468929d43c6d8cb3badd865ec2c7470de211646b1a4

.. code-block:: cpp

    /// Parse floating-point measurements and select their output notation and precision.
    void floatingPointValues() {
        const auto humidity = el::String{"48.75"_el}.toFloatOrThrow<double>();

        auto scientificInput = el::FloatParseOptions{};
        scientificInput.setStyle(el::FloatParseOptions::Style::Scientific);
        const auto pressure = el::String{"1.013e3"_el}.toFloatOrThrow<double>(scientificInput);

        auto valueWithUnit = el::FloatParseOptions{};
        valueWithUnit.addFlags(el::FloatParseFlag::IgnoreTrailingChars);
        const auto temperature = el::String{"21.5 °C"_el}.toFloatOrThrow<double>(valueWithUnit);
        const auto missing = el::String{"geen meting"_el}.toFloat<double>(-1.0);

        el::io::printLine("Humidity ..........: "_el, humidity);
        el::io::printLine("Pressure ..........: "_el, pressure);
        el::io::printLine("Temperature .......: "_el, temperature);
        el::io::printLine("Missing ...........: "_el, missing, " (fallback)"_el);

        try {
            const auto unexpectedValue = el::String{"21,5"_el}.toFloatOrThrow<double>();
            el::io::printLine("21,5 ..............: "_el, unexpectedValue);
        } catch (const el::Exception &) {
            el::io::printLine("21,5 ..............: parse error"_el);
        }

        auto fixed = el::FloatFormat::fixed();
        fixed.setPrecision(el::ItemCount{2U});
        auto scientific = el::FloatFormat::scientific();
        scientific.setPrecision(el::ItemCount{3U}).setLetterCase(el::LetterCase::Uppercase);

        el::io::printLine("Fixed format ......: "_el, el::String::fromFloat(humidity, fixed));
        el::io::printLine("Scientific ........: "_el, el::String::fromFloat(pressure, scientific));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Humidity ..........: 48.75
    Pressure ..........: 1013
    Temperature .......: 21.5
    Missing ...........: -1 (fallback)
    21,5 ..............: parse error
    Fixed format ......: 48.75
    Scientific ........: 1.013E+03

.. erbsland-demo-end::

The Same Model at Every String Width
====================================

The UTF-8 ``String`` examples represent the usual application boundary, but ``U8String``, ``U16String``, ``U32String``,
and their editor counterparts provide the same conversions.
Prefer a read-only string for an input value and reserve an editor for a workflow that is already mutable.

The standalone ``toString()`` overloads provide a convenient route to the common UTF-8 ``String`` when no width-specific
factory is needed.
For output that combines several values, avoid creating one temporary string per scalar: append the typed values to an
``AnyStringBuilder`` or pass them directly to :doc:`using_string_format`.
