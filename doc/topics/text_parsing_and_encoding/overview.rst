..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Parsing and Encoding; Overview

**********************************
Text Parsing and Encoding Overview
**********************************

Parsing and encoding are boundary decisions: the input syntax, accepted errors, and output form all matter.
These topics connect common data formats to the tools that read and write them.

Converting Text and Scalar Values
=================================

Configuration and protocol values often cross a boundary between text and Booleans, integers, or floating-point numbers.
:doc:`/topics/text_parsing_and_encoding/converting_text_and_scalar_values` explains fallback and throwing conversion, accepted input syntax, overflow,
and the formatting options that create text for a particular destination.

Recognizing Small Text Patterns
===============================

Some boundaries are more expressive than a fixed prefix but do not justify a full parser.
:cpp:class:`StringPattern <erbsland::text::StringPattern>` describes these small shapes with literals, character ranges,
and a front/back divider.
:doc:`/topics/text_parsing_and_encoding/using_string_patterns` explains where patterns fit and how they match, trim, split, and return native positions.

Converting Strings and Encoded Data
===================================

Changing string width, encoding text into bytes, reading a text stream, and retaining a width-erased value are distinct
boundary operations.
:doc:`/topics/text_parsing_and_encoding/conversion` separates those workflows and explains ``StringConverter``, encoders and decoders, byte-order marks,
strict and tolerant handling, streams, ``AnyString``, and width-independent output.

Encoding Internationalized Names
================================

Internationalized domain names require more than a Unicode-to-ASCII codec.
IDNA2008 adds normalization, contextual, directionality, and DNS rules around the underlying Punycode representation.
:doc:`/topics/text_parsing_and_encoding/encoding_internationalized_names` explains when raw Punycode is appropriate and how to preserve useful failure
information at a domain-name boundary.

Encoding Byte Data as Base-N Text
=================================

Binary bytes sometimes need to travel through text-only fields without losing their exact value.
:doc:`/topics/text_parsing_and_encoding/encoding_byte_data_as_base_n` introduces Base16, Base32, and Base64 with a complete round trip.
Continue with :doc:`/topics/text_parsing_and_encoding/base_n_encoding` for alphabet, padding, and line-layout choices,
or :doc:`/topics/text_parsing_and_encoding/base_n_decoding` for validation, size limits, and input policy.

Parsing HTML Fragments into Documents
=====================================

When short descriptions contain headings, emphasis, or links in HTML, they can become structured text without tying
their presentation to the source markup.
:doc:`/topics/text_parsing_and_encoding/parsing_html_fragments` shows how to turn a fragment into a ``TextDocument`` and render the same content with
different terminal styles.

Parsing and Rendering JSON Data
===============================

JSON carries structured values between applications and files.
:doc:`/topics/text_parsing_and_encoding/parsing_and_rendering_json` follows a value from parsing through navigation and construction to serialization,
with practical limits for untrusted input and options for readable output.

Parsing Compact Named-Key Parameters
====================================

Short option fields often read better when every value names its purpose.
:doc:`/topics/text_parsing_and_encoding/parsing_named_key_parameters` shows how to register a vocabulary, parse a
comma-separated block, and turn its entries into application options.
Continue with :doc:`/topics/text_parsing_and_encoding/configuring_named_key_parameters` to choose aliases, entry
policies, and value limits, then :doc:`/topics/text_parsing_and_encoding/customizing_named_key_syntax` to adapt
delimiters, prefixes, compact values, and the accepted value alphabet.
