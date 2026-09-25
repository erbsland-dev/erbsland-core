..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Placeholders; Text Filters
    single: Text; Placeholder Filters
    single: Text Filters; Text Placeholders

***************************************************
Transforming Placeholder Text with Built-in Filters
***************************************************

Placeholder sources often return text that is technically valid but not yet suitable for its destination.
It may contain surrounding whitespace, an application prefix, a long diagnostic label, or markup characters that need
escaping.
The built-in text filters handle these common transformations directly in a placeholder expression and can also select
or reject a value based on a small condition.

Enabling the Filter Set
=======================

Call :cpp:func:`Replacer::addTextFilters() <erbsland::text::placeholder::Replacer::addTextFilters>` before replacing
text that uses the built-in filters.
This registers ``safe``, ``trim``, ``slice``, ``remove``, ``replace``, ``escape``, ``if``, ``error_if``, ``default``,
``required``, ``lower``, and ``upper`` on that replacer.
You still need at least one placeholder source, such as the :doc:`built_in_sources` ``env`` source, to begin each
expression.

Filters run from left to right, so each filter receives the result of the previous one.
In ``|trim|remove:text=-draft|safe:40``, the value is first trimmed, then has every exact ``-draft`` removed, and only
then is converted into a bounded safe representation.

Reading Filter Parameters
=========================

Parameters follow the filter name after a colon.
Most filters use named entries separated by commas, with an equals sign between a key and its value:

.. code-block:: text

    ${var:result_label|slice:start=4,length=12}

Parameter keys and enumerated values such as ``front`` or ``html`` are case-insensitive.
Text used for searching, replacement, and conditional results preserves its spelling and remains case-sensitive.
The ``start`` and ``length`` keys have the compact aliases ``s`` and ``l``; a numeric alias can omit the equals sign, as
in ``s4,l12``.
Indexes are zero-based, and both indexes and lengths count Unicode code points rather than UTF-8 bytes.

The placeholder syntax still frames the complete parameter.
If a pipe or closing brace belongs to parameter text, escape it using the chosen mode described in
:doc:`customize_placeholder_syntax`.
A comma separates named entries and therefore cannot be used as literal text in this parameter form.

Cleaning and Selecting Text
===========================

The first group of filters cleans a value or selects a part of it.
This demo uses a volcanic field note to show a left-to-right chain, a code-point slice, range removal, and a bounded
safe string.

.. erbsland-demo::
    :source: text/Placeholders/BuiltInFilters.cpp
    :function-blocks: transformFieldNote
    :function-blocks-sha256: b5c6e77f265e51be0e930ea4adaf946cd5cfec9fc90a1b05fcf7f41a576549b3
    :exec: text/text_placeholders --demo TransformFieldNote
    :source-sha256: e25ca73265f7e769727e6c258a4af57fecb2aaad9862156bfa8761ac84dc6dc5

.. code-block:: cpp

    void transformFieldNote() {
        auto replacer = el::placeholder::Replacer{};
        replacer.setVariableSource(
            {{{"site"_el, u8"  Vulkan_Askja-draft  "_el},
                {"sample"_el, "obs-2026-basalt"_el},
                {"description"_el, u8"Vulkansk aktivitet ved Askja"_el},
                {"status"_el, ""_el}}});
        replacer.addTextFilters();

        el::io::printLine(
            replacer.replaceOrThrow("Site: ${var:site|trim|remove:text=-draft|replace:text=_,rep=-|lower}"_el));
        el::io::printLine(replacer.replaceOrThrow("Year: ${var:sample|slice:start=4,length=4}"_el));
        el::io::printLine(replacer.replaceOrThrow("Sample: ${var:sample|remove:side=front,length=9|upper:ascii}"_el));
        el::io::printLine(replacer.replaceOrThrow("Summary: ${var:description|safe:length=18}"_el));
        el::io::printLine(replacer.replaceOrThrow("Status: ${var:status|default:pending}"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Site: vulkan-askja
    Year: 2026
    Sample: BASALT
    Summary: "Vulka…(28 total)"
    Status: pending

.. erbsland-demo-end::

Creating Bounded Safe Text
--------------------------

``safe`` converts the current value with :cpp:func:`String::toSafeString() <erbsland::text::U8String::toSafeString>`.
It creates a display-oriented representation that escapes unsafe characters and crops long text without splitting an
escape sequence.
The default maximum is 1024 rendered code points.

.. list-table:: ``safe`` parameters
    :header-rows: 1
    :widths: 24 48 28

    * - Parameter
      - Meaning
      - Example
    * - ``length=N`` or ``l=N``
      - Limit the rendered result to ``N`` code points.
      - ``safe:length=40``
    * - ``N``
      - Positional shorthand for the maximum length.
      - ``safe:40``

Only one length form may be present.
The result follows the default safe-string policy, including automatic quotes and a crop annotation when needed.

Trimming the Ends
-----------------

``trim`` removes ASCII whitespace from both ends by default.
You can limit trimming to one end or select another set of characters.

.. list-table:: ``trim`` parameters
    :header-rows: 1
    :widths: 24 48 28

    * - Parameter
      - Meaning
      - Example
    * - ``side=both``
      - Trim both ends. This is the default.
      - ``trim``
    * - ``side=front``
      - Trim only the beginning.
      - ``trim:side=front``
    * - ``side=back``
      - Trim only the end.
      - ``trim:side=back``
    * - ``chars=[pattern]``
      - Trim characters described by a :cpp:func:`CharSet::fromPattern()
        <erbsland::text::CharSet::fromPattern>` pattern instead of whitespace.
      - ``trim:chars=[./]``

``side`` and ``chars`` may be combined.
The pattern inside the brackets uses the character-set syntax described in
:doc:`../text_strings/working_with_character_sets`.

Keeping a Slice
---------------

``slice`` keeps a code-point range and discards the rest.
The direct range form is the most useful when a field contains a known prefix or fixed-width component.
The side forms make it convenient to retain text relative to either end or a split point.

.. list-table:: ``slice`` forms
    :header-rows: 1
    :widths: 34 42 24

    * - Parameters
      - Result
      - Example
    * - ``start=N`` or positional ``N``
      - Keep everything from index ``N``.
      - ``slice:4``
    * - ``start=N,length=M``
      - Keep at most ``M`` code points from index ``N``.
      - ``slice:s4,l8``
    * - ``side=front,length=N``
      - Keep the first ``N`` code points.
      - ``slice:side=front,l=8``
    * - ``side=back,length=N``
      - Keep the last ``N`` code points.
      - ``slice:side=back,l=8``
    * - ``side=front,start=N``
      - Keep the text before split index ``N``.
      - ``slice:side=front,s=8``
    * - ``side=back,start=N``
      - Keep the text from split index ``N``.
      - ``slice:side=back,s=8``

Without ``side``, ``start`` is required and ``length`` is optional.
With ``side``, choose ``front`` or ``back`` and specify exactly one of ``start`` or ``length``.

Removing Text
-------------

``remove`` can delete a code-point range, every exact text match, or every character in a character set.
Choose one of these modes for each filter invocation.

.. list-table:: ``remove`` forms
    :header-rows: 1
    :widths: 34 42 24

    * - Parameters
      - Result
      - Example
    * - ``text=value``
      - Remove every exact, case-sensitive occurrence.
      - ``remove:text=-draft``
    * - ``chars=[pattern]``
      - Remove every character in the pattern set.
      - ``remove:chars=[-_]``
    * - ``start=N`` or positional ``N``
      - Remove everything from index ``N``.
      - ``remove:8``
    * - ``start=N,length=M``
      - Remove at most ``M`` code points from index ``N``.
      - ``remove:s=4,l=8``
    * - ``side=front,length=N``
      - Remove the first ``N`` code points.
      - ``remove:side=front,l=8``
    * - ``side=back,length=N``
      - Remove the last ``N`` code points.
      - ``remove:side=back,l=8``
    * - ``side=front,start=N``
      - Remove the text before split index ``N``.
      - ``remove:side=front,s=8``
    * - ``side=back,start=N``
      - Remove the text from split index ``N``.
      - ``remove:side=back,s=8``

The same range rules used by ``slice`` apply to the range-removal forms.
``text`` and ``chars`` cannot be combined with another parameter.

Replacing Exact Text
--------------------

``replace`` replaces every exact, case-sensitive occurrence of ``text`` with ``rep``.
Both parameters are required, and no other parameter is accepted:

.. list-table:: ``replace`` parameters
    :header-rows: 1
    :widths: 24 48 28

    * - Parameter
      - Meaning
      - Example
    * - ``text=value``
      - Text to find.
      - ``text=_``
    * - ``rep=value``
      - Replacement inserted for every match.
      - ``rep=-``

For example, ``replace:text=_,rep=-`` turns ``wave_energy`` into ``wave-energy``.

Supplying Defaults and Requiring Values
---------------------------------------

``default:<text>`` returns the current value when it is not empty and returns ``<text>`` otherwise:

.. code-block:: text

    Region: ${var:deployment_region|default:local}

The default parameter is literal placeholder text and may contain commas.
It is only selected for empty text; the ``env`` source's missing marker ``undefined`` is not empty.
Here, ``deployment_region`` must exist in the application variable map.
An empty mapped value selects ``local``; an unknown ``var`` name remains an error.

``required`` accepts no parameter.
It returns non-empty text unchanged and raises a ``Validation`` replacer error for empty text:

.. code-block:: text

    Token: ${env:APPLICATION_TOKEN,required|required}

In this example, the source flag rejects a missing environment variable and the filter rejects an existing variable
whose value is empty.

Converting Letter Case
----------------------

``lower`` and ``upper`` convert letter case.
Without a parameter, both use Unicode simple case conversion.
The optional mode ``unicode`` selects that behavior explicitly, while ``ascii`` changes only ASCII letters:

.. code-block:: text

    ${var:display_name|lower}
    ${var:protocol|upper:ascii}

Characters without a simple one-code-point mapping remain unchanged.
An unknown mode produces a ``Syntax`` replacer error.

Escaping, Choosing, and Validating
==================================

The remaining filters prepare text for another syntax or make a small decision from its content.
The next demo escapes an HTML fragment, chooses an audience from a review state, and verifies a sample identifier.

.. erbsland-demo::
    :source: text/Placeholders/BuiltInFilters.cpp
    :function-blocks: chooseFieldNoteText
    :function-blocks-sha256: 83d29fe97076726fb1f31c9d598993c0637a2e7f6c9fe78018044ca45b884ee7
    :exec: text/text_placeholders --demo ChooseFieldNoteText
    :source-sha256: e25ca73265f7e769727e6c258a4af57fecb2aaad9862156bfa8761ac84dc6dc5

.. code-block:: cpp

    void chooseFieldNoteText() {
        auto replacer = el::placeholder::Replacer{};
        replacer.setVariableSource(
            {{{"description"_el, u8"<krater>Askja</krater>"_el}, {"state"_el, "reviewed"_el}, {"sample"_el, "A-104"_el}}});
        replacer.addTextFilters();

        el::io::printLine(replacer.replaceOrThrow("HTML: ${var:description|escape:format=html,amount=required}"_el));
        el::io::printLine(
            replacer.replaceOrThrow("Audience: ${var:state|if:contains=review,then=internal,else=public}"_el));
        el::io::printLine(replacer.replaceOrThrow("Sample: ${var:sample|error_if:empty}"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    HTML: &lt;krater&gt;Askja&lt;/krater&gt;
    Audience: internal
    Sample: A-104

.. erbsland-demo-end::

Escaping for a Target Syntax
----------------------------

``escape`` converts the current value with :cpp:func:`String::toEscaped() <erbsland::text::U8String::toEscaped>`.
The required ``format`` names the destination syntax, while the optional ``amount`` controls how broadly characters are
escaped.

.. list-table:: ``escape`` parameters
    :header-rows: 1
    :widths: 24 48 28

    * - Parameter
      - Accepted values
      - Default
    * - ``format``
      - ``none``, ``html``, ``json``, ``cpp``, ``xml``, ``regex``, ``display``, ``log``, ``config``,
        ``config_test``, or ``markdown``.
      - Required
    * - ``amount``
      - ``nothing``, ``required``, ``balanced``, ``non-ascii``, or ``all``.
      - ``balanced``

The result is escaped text; the filter does not add the surrounding quotes or container required by the destination
syntax.

Testing a Condition
-------------------

``if`` and ``error_if`` share the same conditions.
Exactly one condition must be present, and lengths count Unicode code points.

.. list-table:: Conditions
    :header-rows: 1
    :widths: 28 48 24

    * - Condition
      - True when
      - Example
    * - ``empty``
      - The current text is empty. This condition has no value.
      - ``empty``
    * - ``contains=text``
      - The current text contains an exact, case-sensitive match.
      - ``contains=review``
    * - ``length_eq=N``
      - The current text has exactly ``N`` code points.
      - ``length_eq=8``
    * - ``length_gt=N``
      - The current text has more than ``N`` code points.
      - ``length_gt=80``
    * - ``length_lt=N``
      - The current text has fewer than ``N`` code points.
      - ``length_lt=4``

``if`` requires ``then`` and ``else`` in addition to the condition.
It returns the selected parameter text rather than the original value:

.. code-block:: text

    Audience: ${var:review_state|if:contains=review,then=internal,else=public}

``error_if`` accepts only the condition.
It returns the original value when the condition is false and raises a ``Validation`` replacer error when the condition
is true.
With strict replacement, the error includes the code-point offset of the expression in the original text.

The following demo uses the simpler ``required`` filter to reject an empty application value.

.. erbsland-demo::
    :source: text/Placeholders/BuiltInFilters.cpp
    :function-blocks: requireFieldNoteValue
    :function-blocks-sha256: 05b1e6716d4f2cfb7c13450ed3190df27da006f41df8dd82fd782d7bc447d00e
    :exec: text/text_placeholders --demo RequireFieldNoteValue
    :source-sha256: e25ca73265f7e769727e6c258a4af57fecb2aaad9862156bfa8761ac84dc6dc5

.. code-block:: cpp

    void requireFieldNoteValue() {
        auto replacer = el::placeholder::Replacer{};
        replacer.setVariableSource({{{"sample"_el, ""_el}}});
        replacer.addTextFilters();

        try {
            [[maybe_unused]] const auto result = replacer.replaceOrThrow("Sample: ${var:sample|required}"_el);
        } catch (const el::placeholder::ReplacerError &error) {
            el::io::printLine("Required sample failed: "_el, error.reason());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Required sample failed: A required placeholder value is empty.

.. erbsland-demo-end::

Parameter mistakes, such as combining two conditions or omitting a required key, produce a ``Syntax`` replacer error.
Calling ``addTextFilters()`` twice, or registering a custom filter under one of the built-in names, raises
:cpp:class:`LogicError <erbsland::err::LogicError>` because filter names must be unique.
