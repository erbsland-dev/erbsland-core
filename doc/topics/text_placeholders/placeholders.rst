..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text; Placeholders
    single: Placeholders; Replacing Text
    single: Replacer; First Steps

*********************************
Completing Text with Placeholders
*********************************

Suppose your application keeps a gallery label as text, but the featured movement changes with the exhibition.
You can leave a named place in the label and fill it when you display the text.
The label stays readable, while the application decides which values may enter it.

This page introduces placeholder expressions, their sources and filters, and the
:cpp:class:`Replacer <erbsland::text::placeholder::Replacer>` that brings them together.
The following pages show how to :doc:`extend_placeholders` with application providers and
:doc:`customize_placeholder_syntax` for a particular text format.

A Small Expression in Ordinary Text
===================================

The default expression looks like ``${var:movement|upper}``.
The ``${`` and ``}`` mark its boundaries.
``var`` names a *source*, ``movement`` is its parameter, and ``upper`` is a *filter*.
The source obtains text; the filter changes that text before it is inserted.
Both the parameter and the filter are optional, though the source name is required.

For example, a label could contain ``Gallery: ${var:movement}``.
The application supplies the value of ``movement`` through the built-in ``var`` source.
If the value is ``Ιμπρεσιονισμός``, only the expression is replaced; the ``Gallery:`` prefix remains as written.

Names of sources and filters are case-insensitive.
They follow the regular ELCL name rules: ASCII letters, digits, spaces, and underscores, with a letter first.
Spaces and underscores normalize together, and names are limited to 100 Unicode code points.
Parameters keep their case and are interpreted by the selected provider.
Thus ``var`` treats its parameter as a variable name, while ``env`` treats it as an environment-variable name.

The same expression style also appears in quoted ELCL configuration text.
There, :doc:`/topics/conf/placeholders` explains when the configuration parser expands it and how ELCL escaping applies.
The standalone replacer on this page works on an ordinary :cpp:type:`String <erbsland::text::String>`.

Replace a Label
===============

Create a replacer, register at least one source, and pass it the text to complete.
Here the application owns a small variable map and enables the built-in text filters.
The same replacer can process more than one related label.

.. erbsland-demo::
    :source: text/Placeholders/PlaceholderDemos.cpp
    :function-blocks: replaceGalleryLabel
    :function-blocks-sha256: 6a2ac04f2bf96dddfe8bf42e1a447415ac7e78bc67a7b7c7c9aa6b686f7f9765
    :exec: text/text_placeholders --demo ReplaceGalleryLabel
    :source-sha256: 16fb33a7e417b945628738d032f02b150acb8175e2921395aaa0bf4a98f324d0

.. code-block:: cpp

    void replaceGalleryLabel() {
        auto replacer = el::placeholder::Replacer{};
        replacer.setVariableSource({{{"movement"_el, u8"Ιμπρεσιονισμός"_el}}});
        replacer.addTextFilters();

        const auto label = replacer.replaceOrThrow(u8"Gallery: ${var:movement} (${var:movement|upper})"_el);
        el::io::printLine(label);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Gallery: Ιμπρεσιονισμός (ΙΜΠΡΕΣΙΟΝΙΣΜΌΣ)

.. erbsland-demo-end::

``replaceOrThrow()`` is a good choice when a required label must be complete.
It reports an unknown source, an unknown filter, malformed syntax, or a provider failure as
:cpp:class:`ReplacerError <erbsland::text::placeholder::ReplacerError>`.
The error includes a category and, when available, a zero-based Unicode code-point offset in the original input.

A newly constructed replacer has no sources or filters.
Even ``validate()`` and ``replace()`` require at least one registered source; without one, they raise
:cpp:class:`LogicError <erbsland::err::LogicError>`.
Registering a filter alone cannot make an expression useful because every expression starts with a source.

Sources Answer Requests
=======================

A *source* decides where a value comes from.
The built-in ``var`` source reads an application-supplied map.
The built-in ``env`` source reads process environment variables when replacement runs.
Their names are deliberately explicit in the expression, so a reader can see where each value originates.

You can register them with ``setVariableSource()`` and ``addEnvironmentSource()``.
An optional custom name lets one application use a more specific vocabulary.
Calling ``setVariableSource()`` again with the same name replaces the complete map, which is useful when a new document
or request needs a different set of values.

The next demo uses both sources.
It sets an environment variable for the example and removes it afterward.
The source's ``,required`` flag means replacement fails if that variable is absent.
Without the flag, a missing environment variable yields the literal text ``undefined``.

.. erbsland-demo::
    :source: text/Placeholders/PlaceholderDemos.cpp
    :function-blocks: useBuiltInProviders
    :function-blocks-sha256: 710fe262e1a54494c320d10bbc085071a37814924b1446b1d48cd367a8fb245a
    :exec: text/text_placeholders --demo UseBuiltInProviders
    :source-sha256: 16fb33a7e417b945628738d032f02b150acb8175e2921395aaa0bf4a98f324d0

.. code-block:: cpp

    void useBuiltInProviders() {
        auto environment = el::system::EnvironmentVariables{};
        environment.setOrThrow("ERBSLAND_DEMO_ART_MOVEMENT"_el, u8"Ντανταϊσμός"_el);
        auto replacer = el::placeholder::Replacer{};
        replacer.setVariableSource({{{"movement"_el, u8"  Κυβισμός  "_el}}});
        replacer.addEnvironmentSource();
        replacer.addTextFilters();

        el::io::printLine(replacer.replaceOrThrow("Movement: ${var:movement|trim}"_el));
        el::io::printLine(replacer.replaceOrThrow("Next movement: ${env:ERBSLAND_DEMO_ART_MOVEMENT,required}"_el));
        environment.removeOrThrow("ERBSLAND_DEMO_ART_MOVEMENT"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Movement: Κυβισμός
    Next movement: Ντανταϊσμός

.. erbsland-demo-end::

The ``env`` source normally removes control and format characters other than tabs and newlines from a present value.
Its ``,unsafe_raw`` flag returns the raw environment text when that is genuinely required.
The built-in sources and their lookup rules are described in :doc:`built_in_sources`.

Filters Shape the Result
========================

A *filter* receives the source text and returns new text.
In ``${var:movement|trim|upper}``, ``trim`` removes surrounding whitespace, then ``upper`` converts the trimmed result.
Order matters: each filter sees the output of the previous one.

``addTextFilters()`` makes the built-in filters available to the replacer.
They include trimming, slicing, case conversion, defaults, escaping for another syntax, and value checks.
You can read their parameters and behavior in :doc:`built_in_filters`.
Those filters are shared with the ELCL parser, although escaping the *placeholder expression itself* differs between
ordinary text and ELCL input.

The replacement value is inserted literally.
If a source returns ``${var:other}``, the replacer does not scan that returned text a second time.
This makes the set of requested lookups visible in the original label.

When an Expression Fails
========================

While editing a draft, keeping an unresolved expression can be more useful than stopping at the first problem.
``replace()`` preserves a failed expression exactly as written and continues to later expressions.
``replaceOrThrow()`` instead stops at the first failure.

``validate()`` returns whether the expressions and provider parameters are acceptable, and ``validateOrThrow()`` reports
the first validation failure.
Validation does not promise that a later lookup will succeed: the built-in ``var`` source accepts a valid variable name
even when that key is missing from the current map.
Likewise, an environment variable may disappear between validation and replacement.
Use strict replacement for the final result when a value is required.

An expression must close on the same physical line and may have at most 16 filters.
The next page, :doc:`extend_placeholders`, shows how a custom provider reports lookup or transformation failures and how
it can validate its own parameters.
