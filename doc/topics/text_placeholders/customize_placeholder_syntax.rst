..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Placeholders; Custom Syntax
    single: Placeholders; Escaping
    single: ReplacerOptions

*****************************************
Choosing Placeholder Syntax for Your Text
*****************************************

The default ``${var:name|trim}`` form is easy to recognize in ordinary text.
It may be awkward when the surrounding format already gives ``${...}`` another meaning, or when a label must show that
sequence literally.
:cpp:class:`ReplacerOptions <erbsland::text::placeholder::ReplacerOptions>` lets you choose the expression frame,
the two separators, and the escape mode before constructing a
:cpp:class:`Replacer <erbsland::text::placeholder::Replacer>`.

This page explains each choice and the situations that can make a replacement surprising.
The preceding pages introduce :doc:`placeholders` and show how to :doc:`extend_placeholders`.

Choose a Frame
==============

``setFrame(begin, end)`` changes the opening and closing strings.
For a gallery caption that already contains dollar signs, ``[[var=movement/upper]]`` can be easier to distinguish from
other text.
In that expression, ``[[`` starts the request and ``]]`` ends it.

.. erbsland-demo::
    :source: text/Placeholders/PlaceholderDemos.cpp
    :function-blocks: chooseSyntax
    :function-blocks-sha256: 78b277ff356cb0bf4a46647680e9a95f01f313e74017f6d1e7b5a19f2e452884
    :exec: text/text_placeholders --demo ChooseSyntax
    :source-sha256: 16fb33a7e417b945628738d032f02b150acb8175e2921395aaa0bf4a98f324d0

.. code-block:: cpp

    void chooseSyntax() {
        auto options = el::placeholder::ReplacerOptions{};
        options.setFrame("[["_el, "]]"_el).setNameSeparator("="_el).setFilterSeparator("/"_el);
        auto replacer = el::placeholder::Replacer{options};
        replacer.setVariableSource({{{"movement"_el, u8"Εξπρεσιονισμός"_el}}});
        replacer.addTextFilters();

        el::io::printLine(replacer.replaceOrThrow("[[var=movement/upper]]"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    ΕΞΠΡΕΣΙΟΝΙΣΜΌΣ

.. erbsland-demo-end::

Each frame string must contain 1–16 Unicode code points.
An empty frame or a longer one raises :cpp:class:`ParameterError <erbsland::err::ParameterError>` when you set it.
The options are checked again when the replacer is constructed, because some combinations conflict with the selected
escape mode.

Separate Filters
================

``setFilterSeparator()`` changes what divides filters.
The example uses ``/`` so ``[[var=movement/upper]]`` reads the variable and then runs ``upper``.
The default separator is ``|``.

A filter separator may contain 0–16 code points.
Setting it to an empty string disables filter chains, which is useful when the text format should allow only direct
values.
With filters disabled, a filter-like character in a source parameter is ordinary parameter text unless it conflicts with
the closing frame.

Separate Names from Parameters
==============================

``setNameSeparator()`` changes the boundary after each source or filter name.
The default ``:`` makes ``${var:movement}``; the example changes it to ``=`` for ``[[var=movement]]``.
The same separator applies to filter parameters, such as ``[[var=movement/escape=format=html]]``.

Like the filter separator, it may contain 0–16 code points.
An empty name separator disables parameters altogether.
Use that only when every registered source and filter can work with an empty parameter, because the replacer then has no
way to pass a value such as ``movement`` to ``var``.

The delimiters may overlap.
At a position where more than one matches, the longest matching delimiter wins.
For equal lengths, the closing frame has priority over the filter separator, which has priority over the name separator.
Distinct delimiters are usually easier for the people who will edit the text.

Write a Literal Delimiter
=========================

``setEscapeMode()`` chooses how an author writes a delimiter without starting or ending an expression.
The default :cpp:enumerator:`Backslash <erbsland::text::placeholder::EscapeMode::Backslash>` mode lets a backslash
before the opening frame keep it literal.
Outside an expression, ``\${var:movement}`` becomes the literal ``${var:movement}``, and ``\\`` becomes one backslash.
Other outside backslashes are preserved.

Inside an expression, a backslash can quote the first code point of the closing frame or filter separator.
For example, ``${var:name\|part}`` passes ``name|part`` as one source parameter, while ``${var:name\}part}`` passes
``name}part``.
A doubled backslash produces one literal backslash.
This quoting acts on the placeholder syntax, before the provider interprets its parameter.

In :cpp:enumerator:`Double <erbsland::text::placeholder::EscapeMode::Double>` mode, repeat the first code point of the
opening frame to show that frame literally.
With the default frame, ``$${var:movement}`` becomes ``${var:movement}``.
Inside an expression, repeat the first code point of the closing frame or filter separator: ``||`` supplies a literal
``|`` to a parameter.
This mode helps when a backslash already has special meaning in the surrounding text format.

.. erbsland-demo::
    :source: text/Placeholders/PlaceholderDemos.cpp
    :function-blocks: chooseEscapeMode
    :function-blocks-sha256: b596f2326323bc7fdee452d4d7d1e1c9b1492bc1afd0de8517e7a4a99e5109c5
    :exec: text/text_placeholders --demo ChooseEscapeMode
    :source-sha256: 16fb33a7e417b945628738d032f02b150acb8175e2921395aaa0bf4a98f324d0

.. code-block:: cpp

    void chooseEscapeMode() {
        auto backslash = el::placeholder::Replacer{};
        backslash.setVariableSource({{{"movement"_el, u8"Κυβισμός"_el}}});
        el::io::printLine(backslash.replaceOrThrow("Literal: \\${var:movement}; value: ${var:movement}"_el));

        auto options = el::placeholder::ReplacerOptions{};
        options.setEscapeMode(el::placeholder::EscapeMode::Double);
        auto doubled = el::placeholder::Replacer{options};
        doubled.setVariableSource({{{"movement"_el, u8"Κυβισμός"_el}}});
        el::io::printLine(doubled.replaceOrThrow("Literal: $${var:movement}; value: ${var:movement}"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Literal: ${var:movement}; value: Κυβισμός
    Literal: ${var:movement}; value: Κυβισμός

.. erbsland-demo-end::

In double mode, no configured delimiter may start with two copies of its first code point.
For example, ``[[`` cannot be the opening frame in that mode: its first ``[`` could mean either the delimiter or its
escape.
Constructing the replacer with such options raises :cpp:class:`LogicError <erbsland::err::LogicError>`.

Deal with Invalid Expressions
=============================

Strict replacement makes a final document predictable: ``replaceOrThrow()`` reports the first failed expression.
Tolerant ``replace()`` is useful when someone is editing a draft; it keeps a failed expression and continues to later
ones.
The following example shows both policies on the same input.

.. erbsland-demo::
    :source: text/Placeholders/PlaceholderDemos.cpp
    :function-blocks: handleInvalidExpression
    :function-blocks-sha256: b49b1d56b954125565a815279aa2bc3a60e0ceeac18d550761b7edca4f947422
    :exec: text/text_placeholders --demo HandleInvalidExpression
    :source-sha256: 16fb33a7e417b945628738d032f02b150acb8175e2921395aaa0bf4a98f324d0

.. code-block:: cpp

    void handleInvalidExpression() {
        auto replacer = el::placeholder::Replacer{};
        replacer.setVariableSource({{{"movement"_el, u8"Ρεαλισμός"_el}}});
        const auto draft = "${var:missing}; ${var:movement}"_el;

        el::io::printLine("Draft: "_el, replacer.replace(draft));
        try {
            [[maybe_unused]] const auto result = replacer.replaceOrThrow(draft);
        } catch (const el::placeholder::ReplacerError &error) {
            el::io::printLine("Strict replacement failed: "_el, error.reason());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Draft: ${var:missing}; Ρεαλισμός
    Strict replacement failed: Placeholder variable is not defined: missing

.. erbsland-demo-end::

A missing closing frame leaves the expression literal through its line or the end of input in tolerant mode.
Expressions cannot cross a physical line, contain a nested opening frame, or have more than 16 filters.
``validate()`` and ``validateOrThrow()`` check the expression and provider parameters before replacement, but they
cannot guarantee that a live source will still provide a value later.

Think About the Destination Text
================================

A replacement is inserted as literal text and is never scanned for more placeholders.
That prevents a source from introducing a new placeholder request.
It also means an intentional second replacement pass can change the result: if a source returns ``${var:other}``, a
second call may interpret it.
Normally, complete each text value in one pass.

The replacer does not automatically escape a source value for its destination.
If a value is inserted into HTML, JSON, or another language, use an appropriate text filter or encode it at the
destination boundary.
For example, the built-in ``escape:format=html`` filter prepares text for HTML content, while another destination needs
its own escape format.
Do not mistake quoting the *expression delimiters* for escaping the *replacement value*.

The same care matters with layers of text parsing.
A C++ string literal, an ELCL quoted value, and the replacer each interpret different characters.
An extra backslash may be consumed by an earlier layer before the replacer sees it.
When placeholders occur in ELCL, follow :doc:`/topics/conf/placeholders` for that parser's escape rules.
For ordinary runtime strings, test the exact text you pass to the replacer and choose one escape mode for the whole
document.
