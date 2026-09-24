.. index::
    single: Placeholders; Text Filters
    single: Configuration; Placeholder Filters
    single: Text Filters; Configuration Placeholders

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

Call :cpp:func:`Parser::enableTextPlaceholderFilters() <erbsland::conf::Parser::enableTextPlaceholderFilters>` before
parsing a document that uses the built-in filters.
This registers ``safe``, ``trim``, ``slice``, ``remove``, ``replace``, ``escape``, ``if``, ``error_if``, ``default``,
``required``, ``lower``, and ``upper`` on that parser.
You still need at least one placeholder source, such as the :doc:`built-in-sources` ``env`` source, to begin each
expression.

Filters run from left to right, so each filter receives the result of the previous one.
In ``|trim|remove:text=-draft|safe:40``, the value is first trimmed, then has every exact ``-draft`` removed, and only
then is converted into a bounded safe representation.

Reading Filter Parameters
=========================

Parameters follow the filter name after a colon.
Most filters use named entries separated by commas, with an equals sign between a key and its value:

.. code-block:: elcl

    label: "${env:RESULT_LABEL|slice:start=4,length=12}"

Parameter keys and enumerated values such as ``front`` or ``html`` are case-insensitive.
Text used for searching, replacement, and conditional results preserves its spelling and remains case-sensitive.
The ``start`` and ``length`` keys have the compact aliases ``s`` and ``l``; a numeric alias can omit the equals sign, as
in ``s4,l12``.
Indexes are zero-based, and both indexes and lengths count Unicode code points rather than UTF-8 bytes.

The placeholder syntax still frames the complete parameter.
If a pipe or closing brace belongs to parameter text, write it with an ELCL Unicode escape as explained in
:doc:`placeholders`.
A comma separates named entries and therefore cannot be used as literal text in this parameter form.

Cleaning and Selecting Text
===========================

The first group of filters cleans a value or selects a part of it.
This demo uses research-result metadata to show a left-to-right chain, a code-point slice, range removal, and a bounded
safe string.

.. erbsland-demo::
    :source: conf/Placeholders/BuiltInFilters.cpp
    :function-blocks: builtInTextTransforms
    :function-blocks-sha256: 24c1e75be3235e98469635254e6f9711caa12003b7d8eb7f595faed7744a8494
    :exec: conf/placeholders --demo BuiltInTextTransforms
    :source-sha256: 95f876979c636cc3ae77832989c4b4206c7ac679ae672819d03bfd475e769a03

.. code-block:: cpp

    void builtInTextTransforms() {
        auto environment = el::system::EnvironmentVariables{};
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_TITLE"_el, "  Κυματική_ενέργεια-debug  "_el);
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_SERIES"_el, "obs-2026-alpha"_el);
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_SUMMARY"_el, "Θερμοκρασία επιφάνειας και αλατότητα"_el);
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_LABEL"_el, ""_el);

        auto parser = el::conf::Parser{};
        parser.enableEnvironmentPlaceholderSource();
        parser.enableTextPlaceholderFilters();
        const auto document = parser.parseTextOrThrow(
            "[result]\n"
            "title: \"${env:ERBSLAND_DEMO_RESEARCH_TITLE|trim|remove:text=-debug|replace:text=_,rep=-|lower}\"\n"
            "year: \"${env:ERBSLAND_DEMO_RESEARCH_SERIES|slice:start=4,length=4}\"\n"
            "series: \"${env:ERBSLAND_DEMO_RESEARCH_SERIES|remove:side=front,length=9|upper:ascii}\"\n"
            "summary: \"${env:ERBSLAND_DEMO_RESEARCH_SUMMARY|safe:length=20}\"\n"
            "label: \"${env:ERBSLAND_DEMO_RESEARCH_LABEL|default:pending}\"\n"_el);

        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_TITLE"_el);
        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_SERIES"_el);
        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_SUMMARY"_el);
        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_LABEL"_el);

        el::io::printLine("Title: "_el, document->getTextOrThrow("result.title"_el));
        el::io::printLine("Year: "_el, document->getTextOrThrow("result.year"_el));
        el::io::printLine("Series: "_el, document->getTextOrThrow("result.series"_el));
        el::io::printLine("Summary: "_el, document->getTextOrThrow("result.summary"_el));
        el::io::printLine("Label: "_el, document->getTextOrThrow("result.label"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Title: κυματική-ενέργεια
    Year: 2026
    Series: ALPHA
    Summary: "Θερμοκρ…(36 total)"
    Label: pending

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
:doc:`../text/working_with_character_sets`.

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

.. code-block:: elcl

    region: "${var:deployment_region|default:local}"

The default parameter is literal decoded placeholder text and may contain commas.
It is only selected for empty text; the ``env`` source's missing marker ``undefined`` is not empty.
In the example, ``deployment_region`` must exist in the application variable map; an empty mapped value selects
``local``, while an unknown ``var`` name remains an error.

``required`` accepts no parameter.
It returns non-empty text unchanged and raises a configuration validation error for empty text:

.. code-block:: elcl

    token: "${env:APPLICATION_TOKEN,required|required}"

In this example, the source flag rejects a missing environment variable and the filter rejects an existing variable
whose value is empty.

Converting Letter Case
----------------------

``lower`` and ``upper`` convert letter case.
Without a parameter, both use Unicode simple case conversion.
The optional mode ``unicode`` selects that behavior explicitly, while ``ascii`` changes only ASCII letters:

.. code-block:: elcl

    normalized: "${var:display_name|lower}"
    protocol: "${var:protocol|upper:ascii}"

Characters without a simple one-code-point mapping remain unchanged.
An unknown mode is reported as a configuration syntax error.

Escaping, Choosing, and Validating
==================================

The remaining filters prepare text for another syntax or make a small decision from its content.
The next demo escapes an HTML fragment, chooses an audience from a review state, and verifies that a result identifier
is present.

.. erbsland-demo::
    :source: conf/Placeholders/BuiltInFilters.cpp
    :function-blocks: builtInTextConditions
    :function-blocks-sha256: 471539e68728913866dedcd37d4e77f1e63ead3b10f13966db34212b1cd8a41e
    :exec: conf/placeholders --demo BuiltInTextConditions
    :source-sha256: 95f876979c636cc3ae77832989c4b4206c7ac679ae672819d03bfd475e769a03

.. code-block:: cpp

    void builtInTextConditions() {
        auto environment = el::system::EnvironmentVariables{};
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_MARKUP"_el, "<result id=\"7\">έτοιμο</result>"_el);
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_STATE"_el, "reviewed"_el);
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_ID"_el, "R-104"_el);

        auto parser = el::conf::Parser{};
        parser.enableEnvironmentPlaceholderSource();
        parser.enableTextPlaceholderFilters();
        const auto document = parser.parseTextOrThrow(
            "[publication]\n"
            "markup: \"${env:ERBSLAND_DEMO_RESEARCH_MARKUP|escape:format=html,amount=required}\"\n"
            "audience: \"${env:ERBSLAND_DEMO_RESEARCH_STATE|if:contains=review,then=internal,else=public}\"\n"
            "result_id: \"${env:ERBSLAND_DEMO_RESEARCH_ID|error_if:empty}\"\n"_el);

        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_MARKUP"_el);
        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_STATE"_el);
        environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_ID"_el);

        el::io::printLine("Markup: "_el, document->getTextOrThrow("publication.markup"_el));
        el::io::printLine("Audience: "_el, document->getTextOrThrow("publication.audience"_el));
        el::io::printLine("Result ID: "_el, document->getTextOrThrow("publication.result_id"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Markup: &lt;result id="7"&gt;έτοιμο&lt;/result&gt;
    Audience: internal
    Result ID: R-104

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

.. code-block:: elcl

    audience: "${env:REVIEW_STATE|if:contains=review,then=internal,else=public}"

``error_if`` accepts only the condition.
It returns the original value when the condition is false and raises a configuration validation error when the condition
is true.
The parser attaches the value name, placeholder location, and available source excerpt to the error.

The following demo uses the simpler ``required`` filter and shows the resulting diagnostic for an empty environment
variable.

.. erbsland-demo::
    :source: conf/Placeholders/BuiltInFilters.cpp
    :function-blocks: builtInFilterValidation
    :function-blocks-sha256: 3b1a7c5d28e2b885229a1db2567ddddee5c88f2fdf1269895dc3af34a8d40a1e
    :exec: conf/placeholders --demo BuiltInFilterValidation
    :exec-exit-code: 1
    :source-sha256: 95f876979c636cc3ae77832989c4b4206c7ac679ae672819d03bfd475e769a03

.. code-block:: cpp

    void builtInFilterValidation() {
        auto environment = el::system::EnvironmentVariables{};
        environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_ID"_el, ""_el);

        auto parser = el::conf::Parser{};
        parser.enableEnvironmentPlaceholderSource();
        parser.enableTextPlaceholderFilters();
        const auto document = parser.parseTextOrThrow(
            "[publication]\n"
            "result_id: \"${env:ERBSLAND_DEMO_RESEARCH_ID|required}\"\n"_el);
        el::io::printLine("Unexpected result ID: "_el, document->getTextOrThrow("publication.result_id"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛


      ␛[1;91mValidating␛[22m ␛[1mthe␛[22m ␛[1mConfiguration␛[22m ␛[1mFailed

      ␛[22;39mA required placeholder value is empty.

    ␛[1mError␛[22m ␛[1mSource:
      ␛[22;90mLine:␛[39m     2
      ␛[90mColumn:␛[39m   13
      ␛[90mPosition:␛[39m 27

    ␛[1mConfiguration␛[22m ␛[1mError␛[22m ␛[1mDetails:
      ␛[22;90mcategory:␛[39m  Validation
      ␛[90mname path:␛[39m publication.result_id

      ␛[90m   1 │ ␛[39m[publication]
      ␛[90m   2 │ ␛[39mresult_id: "␛[91m$␛[39m{env:ERBSLAND_DEMO_RESEARCH_ID|required}"
      ␛[90m     │ ␛[39m            ␛[91m▔␛[0m

.. erbsland-demo-end::

Parameter mistakes, such as combining two conditions or omitting a required key, are reported as configuration syntax
errors at the placeholder that contains them.
Calling ``enableTextPlaceholderFilters()`` twice, or registering a custom filter under one of the built-in names, raises
:cpp:class:`LogicError <erbsland::err::LogicError>` because filter names must be unique.

..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0
