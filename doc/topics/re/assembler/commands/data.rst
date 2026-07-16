
****
Data
****

The ``.data`` command writes data into the current *data section*.

- In the *sequence* section, it appends characters to the current character sequence.
- In the *class* section, it appends character ranges to the currently open character class.

Syntax
======

:expression:`.data "text"`
    Append all characters from the given text.

:expression:`.data 'c'`
    Append a single character.

:expression:`.data 123`
    Append a character with the given decimal Unicode code-point.

:expression:`.data $01a2bf`
    Append a character with the given hexadecimal Unicode code-point.

In a class section, you can also specify ranges:

:expression:`.data 'a' - 'z'`
    Append the range from ``'a'`` to ``'z'``.

:expression:`.data 65 - 90`
    Append the range from ``65`` to ``90``.

Arguments
=========

The ``.data`` command expects exactly one value in the sequence section.

In the class section, it also allows a range of characters written as ``value - value``.

Valid value types are:

- text (``"..."``)
- character (``'...'``)
- decimal code-point (``123``)
- hexadecimal code-point (``$1072``)

Notes
=====

- ``.data`` can only be used in the ``&sequence`` and ``&class`` sections.
- Only one value (or one range) is allowed per ``.data`` line.
- In the sequence section, text is decoded as UTF-8 and each decoded Unicode code-point is appended.
- In the class section:

  - Text adds each decoded character as a *single* range (start == end).
  - For non-text values, you can optionally use ``-`` to specify a range.
  - Ranges are collected for the *currently open* class (see :doc:`Class<class>`).

- The maximum length of a character sequence is ``65534`` code-points.

Examples
========

.. code-block:: text

    .section &sequence
    seq_hello:  .data "hello"
                .data ' '
                .data "world"

    .section &class
    myClass:    .class
                .data "():,"
                .data 'a' - 'z'
                .data 'A' - 'Z'
                .data '0' - '9'

