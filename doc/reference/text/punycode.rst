.. index::
    single: Punycode
    single: IDNA2008
    single: Internationalized Domain Name

*********************
Punycode and IDNA2008
*********************

Pure Punycode
=============

The ``erbsland::text::punycode`` namespace provides the reversible RFC 3492 Bootstring codec.
:cpp:class:`PunycodeEncoder <erbsland::text::punycode::PunycodeEncoder>` accepts Unicode ``String`` input and produces
an ASCII payload, while :cpp:class:`PunycodeDecoder <erbsland::text::punycode::PunycodeDecoder>` performs the reverse
operation.
Default-constructed options select pure Punycode: there is no ``xn--`` prefix handling, domain splitting, normalization,
or character policy.

``encode()`` and ``decode()`` return an empty optional for data-dependent failures.
The ``OrThrow`` variants preserve the exact :cpp:class:`ParseError <erbsland::err::ParseError>` reason.

Strict IDNA2008
===============

:cpp:class:`PunycodeOptions <erbsland::text::punycode::PunycodeOptions>` can select strict IDNA2008 processing for one
label or a complete domain.
The network factory folds ASCII uppercase, normalizes Unicode to NFC, validates Unicode 17 derived properties,
CONTEXTJ/CONTEXTO and bidi rules, verifies A-label round trips, and enforces DNS byte limits.
It deliberately does not apply UTS #46, compatibility or width mappings, Unicode-wide lowercase mapping, or Unicode dot
substitutions.

An optional :cpp:class:`CharSet <erbsland::text::CharSet>` narrows the canonical Unicode characters accepted after the
IDNA2008 checks.
Dots remain domain separators and are not tested by that filter.

See :doc:`/topics/text/encoding_internationalized_names` for practical codec and domain examples.

Interface
=========

.. doxygenclass:: erbsland::text::punycode::PunycodeDecoder
    :members:
.. doxygenclass:: erbsland::text::punycode::PunycodeEncoder
    :members:
.. doxygenenum:: erbsland::text::punycode::PunycodeMode
.. doxygenclass:: erbsland::text::punycode::PunycodeOptions
    :members:
