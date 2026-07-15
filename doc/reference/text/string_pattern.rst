.. index::
    single: String Pattern

**************
String Pattern
**************

Introduction
============

``StringPattern`` is a lightweight pattern matcher for decoded string characters.
It is intended for small internal checks such as protocol or prefix/suffix matching.
The parsed syntax supports literal text, ``?`` for one decoded character, ``[a-z]`` character sets, and one ``*``
divider.
The divider separates front and back matching parts; it is not a repeated wildcard.

Parsed patterns support backslash escapes for ``?``, ``*``, ``[``, ``]`` and ``\``.
Typed construction uses the ``erbsland::text::pattern`` element namespace to build immutable compiled pattern data
directly without parsing pattern syntax.

Example
=======

.. code-block:: cpp

    using namespace el::text::literals;

    const auto parsed = el::StringPattern{"http?://*"_el};
    const auto ok = parsed.matches("https://example.test"_el);

    using namespace el::text::pattern;

    static const auto staticPattern = el::StringPattern{
        Text{U"http"},
        OneChar{},
        Text{U"://"},
        Divider{}};

Interface
=========

.. doxygenstruct:: erbsland::text::pattern::Divider
    :members:
.. doxygenstruct:: erbsland::text::pattern::OneChar
    :members:
.. doxygenclass:: erbsland::text::pattern::Set
    :members:
.. doxygenclass:: erbsland::text::pattern::Range
    :members:
.. doxygenclass:: erbsland::text::pattern::Text
    :members:
.. doxygenclass:: erbsland::text::StringPattern
    :members:
