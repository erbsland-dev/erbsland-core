
*****
Class
*****

The ``.class`` command starts a new character class definition in the ``&class`` section.

Character classes are later referenced from the ``CLASS`` operation.

To define the class contents, use the :doc:`Data<data>` command to add single characters and/or ranges.

Syntax
======

:expression:`.class`
    Close the previous class (if any) and start a new one.

Notes
=====

- ``.class`` can only be used in the ``&class`` section.
- A class must not be empty:

  - If you start a new class while the previous one has no ``.data`` entries, the assembler raises an error.
  - The last class in the file is also validated and must not be empty.

- Data (ranges) are collected between ``.class`` commands.
- The maximum number of character classes is ``65534``.

Examples
========

.. code-block:: text

    .section &class

    digits:    .class
              .data '0' - '9'

    hex:       .class
              .data '0' - '9'
              .data 'a' - 'f'
              .data 'A' - 'F'

