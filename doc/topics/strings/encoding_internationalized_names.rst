.. index::
    single: Punycode
    single: IDNA2008
    single: Domain Name; Internationalized

********************************
Encoding Internationalized Names
********************************

Punycode and IDNA solve related but different problems.
Punycode is a reversible text codec; IDNA2008 adds the normalization, character, context, directionality, and DNS rules
required for domain names.
This page shows how to select the correct boundary and how failures remain visible.

Use Pure Punycode for Text Protocols
====================================

The default options implement RFC 3492 only.
The encoder does not add ``xn--``, and the decoder treats its entire input as one Punycode payload.

.. code-block:: cpp

    using namespace erbsland::text::punycode;

    const auto payload = PunycodeEncoder{"bücher"_el}.encodeOrThrow();
    // payload == "bcher-kva"

    const auto text = PunycodeDecoder{payload}.decodeOrThrow();
    // text == "bücher"

This mode is appropriate when another format already defines where a Punycode payload begins and ends.
ASCII-only input follows RFC 3492 delimiter behavior, so it is not an IDNA label serializer.

Use IDNA2008 for Domain Names
=============================

The network options process every label and return the canonical ASCII transport representation.
Decoding performs the inverse conversion and returns lowercase NFC Unicode.

.. code-block:: cpp

    const auto options = PunycodeOptions::network();
    const auto ascii = PunycodeEncoder{"www.bücher.example"_el, options}.encodeOrThrow();
    // ascii == "www.xn--bcher-kva.example"

    const auto unicode = PunycodeDecoder{ascii, options}.decodeOrThrow();
    // unicode == "www.bücher.example"

Only ASCII uppercase is folded.
Non-ASCII uppercase, compatibility spellings, fullwidth characters, Unicode dot variants, symbols, emoji, malformed
A-labels, contextual-rule failures, and bidi-rule failures are rejected rather than mapped into another spelling.

Choose Failure Reporting Deliberately
=====================================

Use ``encode()`` and ``decode()`` when invalid input is an ordinary alternative in a parser.
Use ``encodeOrThrow()`` and ``decodeOrThrow()`` when the caller must retain the precise security or syntax reason.
All data-dependent failures use :cpp:class:`ParseError <erbsland::err::ParseError>`; allocation failures retain their
ordinary system exception type.
