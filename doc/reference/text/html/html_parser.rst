.. index::
    single: HTML Parser
    single: HtmlParser

***********
HTML Parser
***********

Introduction
============

The HTML parser converts a tolerant subset of HTML fragments or documents into a
:cpp:class:`TextDocument <erbsland::text::TextDocument>`.
Malformed tags and entities are recovered as text where possible, so user-facing parsers can accept imperfect input.

Usage
=====

Create :cpp:class:`HtmlParser <erbsland::text::html::HtmlParser>` with an
:cpp:class:`AnyString <erbsland::text::AnyString>` compatible value and call ``parse()`` for the default
tolerant API.
Use ``parseOrThrow()`` when future unrecoverable parser errors should be reported as
:cpp:class:`ParseError <erbsland::err::ParseError>`.

Interface
=========

.. doxygenclass:: erbsland::text::html::HtmlParser
    :members:
