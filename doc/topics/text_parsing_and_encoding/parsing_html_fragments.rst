..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: HTML; Parsing Fragments
    single: TextDocument; HTML Input
    single: HtmlParser

***************************
Parsing HTML into Documents
***************************

A short description can carry more than words.
It may have a heading, an emphasized phrase, or a link.
If that formatting arrives as HTML, :cpp:class:`HtmlParser <erbsland::text::html::HtmlParser>` turns it into a
:cpp:class:`TextDocument <erbsland::text::TextDocument>`.

The document keeps the text and its structure together.
You can render it with a style that suits your terminal, or inspect its nodes when you need information from the text.

Where HTML Fragments Fit
========================

The parser is particularly useful for *fragments*.
A heading and a few paragraphs can be embedded in an application without wrapping them in ``html`` or ``body`` elements.

Familiar tags give the text its shape: headings and paragraphs, lists, links, emphasis, code, and line breaks.
Character references are decoded too, so ``&amp;`` becomes ``&`` in the document.
If the markup is imperfect, the parser tries to preserve useful text.

Once you have a ``TextDocument``, the source markup no longer dictates its appearance.
The same description can use a compact style in a status view and a more expressive style in a full-screen viewer.

You can also inspect the document's nodes.
That may be enough to extract information from a small, predictable set of HTML inputs.
For arbitrary web pages, however, treat extraction as a best-effort convenience: the parser is designed for text
documents, not for a browser's complete document model.

Parsing Once, Rendering in Different Styles
===========================================

Construct the parser with your HTML text and call ``parse()``.
The resulting document remains available after the parser goes out of scope.

The example starts with a synthesizer-patch description.
It passes the parsed document to
:cpp:class:`TerminalDocumentRenderer <erbsland::cterm::TerminalDocumentRenderer>` twice, choosing a different style
each time.

.. erbsland-demo::
    :source: text/HtmlFragments/RenderFragment.cpp
    :exec: text/html_fragments --demo RenderFragment
    :source-sha256: e539c031f2c7c70b16706c6dd5cda57cc100225dc147db580bc1b6a443401243

.. code-block:: cpp

    /// Parse a small HTML fragment into a text document and render it with terminal styles.
    ///
    /// `HtmlParser` accepts markup embedded in ordinary text and produces a semantic `TextDocument`. The same document
    /// can be rendered with different `TerminalDocumentStyle` presets without parsing the HTML again.
    void renderFragment() {
        // Parse the description of a synthesizer patch. The Turkish words are input data, not parser commands.
        const auto html =
            u8"<h2>Yankı</h2><p>A <strong>bright</strong> patch with <em>soft</em> echoes &amp; a long tail.</p>"_el;
        const auto document = el::html::HtmlParser{html}.parse();

        // Render the same document using a restrained and a more decorative terminal style.
        el::io::printLine("Simple style:"_el);
        el::cterm::TerminalDocumentRenderer{el::cterm::TerminalDocumentStyle::defaultSimple()}.renderTo(
            *el::application().terminal(), document);
        el::io::printLine("Styled style:"_el);
        el::cterm::TerminalDocumentRenderer{el::cterm::TerminalDocumentStyle::defaultStyled()}.renderTo(
            *el::application().terminal(), document);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Simple style:
    ␛[1;96;40mYankı

    ␛[22;37mA ␛[1mbright␛[22m patch with ␛[3msoft␛[23m echoes & a long tail.

    ␛[39;49mStyled style:


       ␛[1;36;40m-◆ ␛[96mYankı␛[36m ◆-␛[90m──────────────────────────────────────────────────────────────────────────

          ␛[22;37mA ␛[1mbright␛[22m patch with ␛[3msoft␛[23m echoes & a long tail.␛[0m

.. erbsland-demo-end::

The heading and emphasized words have become document nodes.
Their appearance comes from the selected
:cpp:class:`TerminalDocumentStyle <erbsland::cterm::TerminalDocumentStyle>`, not from terminal escape sequences in the
HTML input.

The predefined ``defaultSimple()`` and ``defaultStyled()`` styles give you two starting points.
You can also provide a custom style, or call ``document.toString()`` when plain text is all you need.

When the Markup Is Imperfect
============================

For user-facing fragments, ``parse()`` is usually the simplest choice.
It returns the best document the parser can produce, even when it has to recover from damaged markup.

``parseOrThrow()`` behaves the same way for recoverable input.
It can report an unrecoverable parser condition as
:cpp:class:`ParseError <erbsland::err::ParseError>`.

Recovery does not guarantee a browser-equivalent result for damaged or unsupported HTML.
If your application relies on a particular heading, link, or other structure, inspect the resulting nodes before acting
on them.
